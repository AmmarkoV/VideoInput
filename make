#!/bin/bash
echo "Compiling files.."
gcc -c main.cpp -o main.o
gcc -c image_storage.cpp -o image_storage.o
gcc -c V4L2.cpp -o V4L2.o
gcc -c PrintV4L2.cpp -o PrintV4L2.o
gcc -c PixelFormats.cpp -o PixelFormats.o 

echo "Linking files.."
ar  rcs libVideoInput.a main.o image_storage.o V4L2.o PrintV4L2.o PixelFormats.o 

echo "Cleaning compiled object files.."
rm main.o image_storage.o V4L2.o PrintV4L2.o PixelFormats.o

echo "Done.."
exit 0
