#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <limits.h> // For PATH_MAX
#include <errno.h>  // For error handling
#include <libgen.h> // For basename

#define PORT 8080
#define BUFFER_SIZE 4096

sem_t *semaphores;
int client_socket;
int n; // Number of messages

typedef struct {
    int thread_id;
    char *file_name;
} ThreadArgs;



void compute_checksum(const char *file_name, unsigned char *checksum) {


    
    char resolved_path[PATH_MAX];
    if (realpath(file_name, resolved_path) == NULL) {
        perror("Failed to resolve file path");
        close(client_socket);
        // continue;
    }
    
    FILE *file = fopen(resolved_path, "rb");
    if (!file) {
        perror("Failed to open file for checksum computation");
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        perror("Failed to create EVP_MD_CTX");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Initialize the SHA-256 digest context
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        perror("Failed to initialize digest context");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1) {
            perror("Failed to update digest");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);

    unsigned int digest_length;
    if (EVP_DigestFinal_ex(mdctx, checksum, &digest_length) != 1) {
        perror("Failed to finalize digest");
        EVP_MD_CTX_free(mdctx);
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX_free(mdctx);
}



void *send_segments(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    int thread_id = thread_args->thread_id;
    char *file_name = thread_args->file_name;
    // printf("Path: %s\n", file_name);

    // printf("Path: %s\n", resolved_path);

    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        close(client_socket);
        // continue;    
    }
    printf("Thread %d: Opened file %s\n", thread_id, file_name);
    //chunk
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    long chunk_size = (file_size + n - 1) / n; // Round up
    long start_pos = chunk_size * thread_id;
    long end_pos = (thread_id + 1) * chunk_size;

    if (end_pos > file_size) {
        end_pos = file_size; // Adjust for the last chunk
    }

    fseek(file, start_pos, SEEK_SET);
    char buffer[BUFFER_SIZE];
    long current_pos = start_pos;



    // Wait for semaphore for this thread
    sem_wait(&semaphores[thread_id]);
    // Send the file chunk to the client
    while (current_pos < end_pos) {
        long bytes_to_read = BUFFER_SIZE;
        if (current_pos + BUFFER_SIZE > end_pos) {
            bytes_to_read = end_pos - current_pos;
        }

        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
        if (bytes_read <= 0) break;

        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Failed to send file chunk");
            break;
        }
        //clear buffer
        memset(buffer, 0, BUFFER_SIZE);

        current_pos += bytes_read;
    }

    printf("Thread %d: Sent bytes %ld to %ld\n", thread_id, start_pos, current_pos);

    // Signal the next thread's semaphore
    if (thread_id + 1 < n) {
        sem_post(&semaphores[thread_id + 1]);
    }

    free(args);
    pthread_exit(NULL);
}

void setup_server(int* serverPtr,struct sockaddr_in* server_addr_ptr){
    int opt=1;
    // Create server socket
    *serverPtr = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverPtr == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(*serverPtr, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(*serverPtr);
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr_ptr->sin_family = AF_INET;
    server_addr_ptr->sin_addr.s_addr = INADDR_ANY;
    server_addr_ptr->sin_port = htons(PORT);

    // Bind the socket
    if (bind(*serverPtr, (struct sockaddr *)server_addr_ptr, sizeof(*server_addr_ptr)) < 0) {
        perror("Bind failed");
        close(*serverPtr);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(*serverPtr, 10) < 0) {
        perror("Listen failed");
        close(*serverPtr);
        exit(EXIT_FAILURE);
    }
}

int main() {
    char file_name[BUFFER_SIZE]; 
    //Checksum
    unsigned char checksum[EVP_MAX_MD_SIZE]; // Buffer to hold checksum
    
    //Server socket
    int server_socket;
    // int opt=1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    
    setup_server(&server_socket,&server_addr);
    
    printf("Server is listening on port %d...\n", PORT);
    while(1){
        // Accept a client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }


        // Receive the number of messages (n)
        int try = 3;
        while (recv(client_socket, &n, sizeof(n), 0) <= 0 && try>0) {
            perror("Failed to receive number of messages. Trying again\n");
            try--;
            close(client_socket);
            continue; 
        }
        n = ntohl(n); // Convert from network byte order
        // printf("Received number of threads: %d\n", n);

        // Receive file name from the client
        memset(file_name, 0, BUFFER_SIZE); // Clear the buffer
        ssize_t bytes_received = recv(client_socket, file_name, sizeof(file_name), 0);
        if (bytes_received <= 0) {
            perror("Failed to receive file name");
            close(client_socket);
            continue; // Move to the next client instead of exiting
        }
        file_name[bytes_received] = '\0'; // Null-terminate the file name
        printf("Received file name: %s with threads %d \n", file_name, n);
        
        char resolved_path[BUFFER_SIZE];
        if (realpath(file_name, resolved_path) == NULL) {
            perror("Failed to resolve file path");
            close(client_socket);
            continue;
        }
        printf("Resolved path: %s\n", resolved_path);
        char * file_name_check = basename(resolved_path);
        // Compute the checksum of the file & send it to the client
        compute_checksum(file_name_check, checksum);
        
        if (send(client_socket, checksum, 32, 0) == -1) {
            perror("Failed to send checksum");
            close(client_socket);
            continue;
        }

        // Initialize semaphores
        semaphores = malloc(n * sizeof(sem_t));
        for (int i = 0; i < n; i++) {
            sem_init(&semaphores[i], 0, 0);
        }

        // Start the first thread immediately
        sem_post(&semaphores[0]);

        // Create threads
        pthread_t threads[n];
        for (int i = 0; i < n; i++) {
            ThreadArgs *args = malloc(sizeof(ThreadArgs));
            args->thread_id = i;
            args->file_name = resolved_path;
            

            if (pthread_create(&threads[i], NULL, send_segments, args) != 0) {
                perror("Thread creation failed");
                free(args);
            }
        }

        // Wait for all threads to complete
        for (int i = 0; i < n; i++) {
            pthread_join(threads[i], NULL);
            sem_destroy(&semaphores[i]);
        }

        // Clean up
        free(semaphores);
        close(client_socket);
    }
    close(server_socket);

    return 0;
}
