# Heliox Programming Language

**Heliox** is a work-in-progress programming language created with the intention of learning how compilers work.  
> ⚠️ Note: The project is still very much in the early stages of development

Whilst Windows works, its calling convention is not yet supported.
---

## Usage

Before using Heliox, make sure to read the `REQUIREMENTS` file for necessary tools.

### Build and Run

After building, the compiled binary will be found in the `build` folder.

#### Linux
```bash
# Build the project
./build.sh

# Generate a binary from a heliox file
./heliox [file].hlx
nasm -felf64 [file].asm -o [file].o
gcc -no-pie [file].o -o [file]

# Run the binary
./[file]
```

#### Windows
```bash
# Build the project
./build.bat

# Generate a binary from a heliox file
./heliox.exe [file].hlx
nasm -fwin64 [file].asm -o [file].o
gcc -no-pie [file].o -o [file].exe

# Run the binary
./[file].exe
```
