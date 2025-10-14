#!/bin/bash
set -e  # oprește execuția dacă apare o eroare

echo "[1/8] Asamblare stage1.asm (boot sector)"
nasm -f bin -Iboot/includes/ -I. boot/stage1.asm -o boot/bin/stage1.bin

echo "[2/8] Asamblare stage2.asm (ELF obiect)"
nasm -f elf32 -Iboot/includes/ -I. boot/stage2.asm -o build/stage2_asm.o

echo "[3/8] Compilare stage2.c (C 32-bit, freestanding)"
gcc -m32 -ffreestanding -fno-builtin -nostdlib -nostartfiles \
    -fno-pic -fno-pie -fno-stack-protector -O2 \
    -c boot/stage2.c -o build/stage2_c.o

echo "[4/8] Link stage2 (ASM + C → ELF)"
ld -m elf_i386 -Ttext 0x1000 -e start \
    build/stage2_asm.o build/stage2_c.o -o build/stage2.elf

echo "[5/8] Convert ELF → binar pur"
objcopy -O binary build/stage2.elf boot/bin/stage2.bin

echo "[6/8] Creare imagine disc goală (64 MB)"
truncate -s 64M boot/disk.img

echo "[7/8] Scriere stage1 + stage2 în imagine"
dd if=boot/bin/stage1.bin of=boot/disk.img conv=notrunc
dd if=boot/bin/stage2.bin of=boot/disk.img bs=512 seek=1 conv=notrunc

echo "[8/8] Rulare în QEMU"
qemu-system-i386 -drive file=boot/disk.img,format=raw \
    -no-reboot -no-shutdown -d int,cpu_reset
