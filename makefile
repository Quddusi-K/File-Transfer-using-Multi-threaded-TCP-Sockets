# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread
CRYPTO_LIBS = -lcrypto -lssl

# Targets
TARGETS = server client 
# testc tests

# Files created by bash script
FILES = archive.zip corruptedfile.bin image.jpg largefile.txt largeimage.jpg testfile.txt video.mp4 mediumvideo.mp4

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Build all targets
all: $(TARGETS)

# Specific build rules
server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $@ $(LDFLAGS) $(CRYPTO_LIBS)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $@ $(LDFLAGS) $(CRYPTO_LIBS)

# Clean build files
clean:
	rm -f $(TARGETS) $(SERVER_OBJ) $(CLIENT_OBJ) $(FILES)
