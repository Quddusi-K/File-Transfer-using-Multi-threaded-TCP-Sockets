#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <sys/stat.h>
#include <libgen.h>



#define PORT 8080
#define BUFFER_SIZE 4096
#define SERVER_IP "127.0.0.1"  

void compute_checksum(const char *file_name, unsigned char *checksum) {
    FILE *file = fopen(file_name, "rb");
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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file name> <number_of_messages> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[2]);
    if (n <= 0) {
        fprintf(stderr, "Number of threads must be greater than 0\n");
        exit(EXIT_FAILURE);
    }
    char *file_path = argv[1];

    // Extract only the file name from the full path
    char *file_name = basename(file_path);
    // char *file_name = argv[1];
    //Output file name should "received_<file_name>" and should be in the directory named "received_files"
    char *output_dir = "./received_files";

    // Check if the directory exists
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        // Create the directory if it doesn't exist
        if (mkdir(output_dir, 0755) == -1) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
    }
    char output_file[256];
    strcpy(output_file, output_dir);
    strcat(output_file, "/received_");
    strcat(output_file, file_name);

    // char *output_file = argv[3];
    int client_socket;
    struct sockaddr_in server_addr;

    // Create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_socket);
        return -1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    n = htonl(n); // Convert to network byte order
    send(client_socket, &n, sizeof(n), 0);
    // Send file name to the server
    // file_name=file_path;
    // file_name[strlen(file_name)] = '\0'; // Null-terminate the string
    file_path[strlen(file_path)] = '\0'; // Null-terminate the string

    if (send(client_socket, file_path, strlen(file_path) + 1, 0) < 0) {
        perror("Failed to send file name");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    // Send the number of messages (n)

    printf("Sent file name and threads \n");

    FILE* file = fopen(output_file, "wb");
    if (file == NULL) {
        perror("Failed to open file");
        close(client_socket);
        exit(EXIT_FAILURE);
    }


    //receive the checksum from the server
    unsigned char checksum[EVP_MAX_MD_SIZE];
    if (recv(client_socket, checksum, 32, 0) <= 0) {
        perror("Failed to receive checksum");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    // Receive the file from the server
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
 
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, sizeof(char), bytes_received, file);
        memset(buffer, 0, sizeof(buffer));
    }

    if (bytes_received < 0) {
        perror("Error receiving data");
    } else if (bytes_received == 0) {
        printf("Connection closed by server\n");
    }

    fclose(file);
    close(client_socket);

    // Compute the checksum of the file
    unsigned char localChecksum[EVP_MAX_MD_SIZE];
    compute_checksum(output_file, localChecksum);
    printf("Comparing Checksum\n");
    if (memcmp(checksum, localChecksum, 32) == 0) {
        printf("Checksum matched. Files received successfully! :-) \n");
    } else {
        printf("Checksums do not match. File is corrupted.\nPlease try again! :-( \n");
    }

    return 0;
}