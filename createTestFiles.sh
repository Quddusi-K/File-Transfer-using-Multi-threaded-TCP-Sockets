#!/bin/bash

# Create Test Files

# Test 1: Small text file (1 KB)
echo "Creating testfile.txt (1 KB)"
base64 /dev/urandom | head -c 1024 > testfile.txt

# Test 2: Large text file (10 MB)
echo "Creating largefile.txt (10 MB)"
base64 /dev/urandom | head -c $((10 * 1024 * 1024)) > largefile.txt

# Test 3: Small image file (50 KB)
echo "Creating image.jpg (50 KB)"
head -c $((50 * 1024)) /dev/urandom > image.jpg

# Test 4: Large image file (500 KB)
echo "Creating largeimage.jpg (500 KB)"
head -c $((500 * 1024)) /dev/urandom > largeimage.jpg

# Test 5: Small video file (1 MB)
echo "Creating video.mp4 (1 MB)"
head -c $((1 * 1024 * 1024)) /dev/urandom > video.mp4

# Test 6: Moderate video file (10 MB)
echo "Creating mediumvideo.mp4 (10 MB)"
head -c $((10 * 1024 * 1024)) /dev/urandom > mediumvideo.mp4

# Test 7: Compressed zip file (5 MB)
echo "Creating archive.zip (5 MB)"
dd if=/dev/urandom of=random_data bs=1M count=5 &>/dev/null
zip archive.zip random_data &>/dev/null
rm -f random_data

# Test 8: Corrupted file (10 KB)
echo "Creating corruptedfile.bin (10 KB)"
head -c $((10 * 1024)) /dev/urandom > corruptedfile.bin
echo "Corrupted data appended" >> corruptedfile.bin

echo "All test files created successfully."



