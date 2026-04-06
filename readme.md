# Heliox programming language (rework)

This project is a work in progress and was made by me to learn how compilers work.

Currently, the code generation is not finished.


USAGE:

Read the REQUIREMENTS for the requirements to use this program.

How to build the project (the binary is found in the generated build folder):
LINUX:
./build.sh

WINDOWS:
./build.bat

How to generate a binary:

LINUX:
./heliox [file].hx
nasm -felf64 [file].asm -o [file].o
gcc --no-pie [file].o -o [file]
run the file
./[file]

WINDOWS:
./heliox.exe [file].hx
nasm -fwin64 [file].asm -o [file].o
gcc --no-pie [file].o -o [file].exe
run the file
./[file].exe

