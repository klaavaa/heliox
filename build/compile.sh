#!/bin/bash
nasm -f elf64 $1.asm -o $1.obj
gcc $1.obj -o $1
