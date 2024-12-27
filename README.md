### README.md

# File Transfer with Multi-threaded Server and Client

This project implements a multi-threaded server and client for file transfer. The server divides the file into chunks and sends them to the client using multiple threads. The client reassembles the chunks and saves the file.

---

## **How to Compile**

Use the following command to compile both the server and client:
```bash
make all
```

This will generate the `server` and `client` executables.

---

## **How to Run**

### **Server**
Start the server:
```bash
./server
```
The server listens on port `8080` for incoming connections.

### **Client**
Run the client with the following command:
```bash
./client <file_name> <num_threads>
```
- `<file_name>`: The name of the file requested from the server.
- `<num_threads>`: The number of threads for file transfer.

### **Example**
```bash
./client testfile.txt 4 
```

The received file is saved in the directory `./received_files` with the name prefixed by `received_`.

---

## **Testing**

1. **Create Test Files**  
Run the following script to generate test files for different cases i.e. archive.zip corruptedfile.bin image.jpg largefile.txt et cetera:
```bash
./createTestFiles.sh
```
**Using make clean will also remove the files created through this script.**

2. **Test File Transfer**  
Run the following script to test the proper transfer of files:
```bash
./tests.sh
```

The `tests.sh` script automates testing by:
- Starting the server.
- Running the client for each test file created in `createTestFiles.sh`.

---

## **File Directory Structure**
- All received files are stored in the `./received_files` directory.
- Files are saved with the name `received_<file_name>`.

---

## **Dependencies**

Ensure the following tools and libraries are installed:
- GCC (for compiling)
- POSIX threads (pthreads)
- OpenSSL (SHA-256)

---
