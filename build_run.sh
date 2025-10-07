#!/bin/bash

# Oprire la prima eroare
set -e

# Asamblare fișierele NASM
nasm -f bin stage1.asm -o stage1.bin
nasm -f bin stage2.asm -o stage2.bin

# Creare / scriere imagine disk
dd if=stage1.bin of=disk.img conv=notrunc
dd if=stage2.bin of=disk.img bs=512 seek=1 conv=notrunc

# Rulare în QEMU
qemu-system-i386 -drive format=raw,file=disk.img
