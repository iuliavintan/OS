# === CONFIGURARE GENERALĂ ===
ASM      := nasm
CC       := gcc
LD       := ld
OBJCOPY  := objcopy

# CC       := i686-elf-gcc      # prefer cross toolchain
# LD       := i686-elf-ld
# OBJCOPY  := i686-elf-objcopy


QEMU     := qemu-system-i386
LDSCRIPT := boot/link.ld

# === DIRECTOARE ===
BINDIR   := boot/bin
BUILDDIR := build
DISK     := boot/disk.img
DISK_SIZE := 64M
SECTOR    := 512

INCLUDES := -Iboot/includes/ -Isrc/ -I. 

# === SURSE ===
STAGE1_SRC := boot/stage1.asm
STAGE2_ASM := boot/stage2.asm
STAGE2_C   := boot/stage2.c
KERNEL_C   := src/kernel.c
KERNEL_OBJ := $(BUILDDIR)/kernel.o

# === OUTPUT ===
STAGE1_BIN := $(BINDIR)/stage1.bin
STAGE2_ASM_OBJ := $(BUILDDIR)/stage2_asm.o
STAGE2_C_OBJ   := $(BUILDDIR)/stage2_c.o
STAGE2_ELF     := $(BUILDDIR)/stage2.elf
STAGE2_BIN     := $(BINDIR)/stage2.bin

# === FLAGS ===
NASMFLAGS := -f elf32 $(INCLUDES)
CFLAGS    := -m32 -ffreestanding -fno-builtin -nostdlib -nostartfiles -fno-pic -fno-pie -fno-stack-protector -O2 -Wall -Wextra $(INCLUDES)
LDFLAGS   := -m elf_i386 -T $(LDSCRIPT)
# === PHONY TARGETS ===
.PHONY: all run clean dirs

# === REGULA PRINCIPALĂ ===
all: run

# === CREARE DIRECTOARE ===
dirs:
	@mkdir -p $(BINDIR) $(BUILDDIR)

# === COMPILARE ===
$(STAGE1_BIN): $(STAGE1_SRC) | dirs
	$(ASM) -f bin $(INCLUDES) $< -o $@

$(STAGE2_ASM_OBJ): $(STAGE2_ASM) | dirs
	$(ASM) $(NASMFLAGS) $< -o $@

$(STAGE2_C_OBJ): $(STAGE2_C) | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_OBJ): $(KERNEL_C) | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(STAGE2_ELF): $(STAGE2_ASM_OBJ) $(KERNEL_OBJ) | dirs
	$(LD) $(LDFLAGS) $^ -o $@

# $(STAGE2_ELF): $(STAGE2_ASM_OBJ) $(STAGE2_C_OBJ)
# 	$(LD) $(LDFLAGS) $^ -o $@

$(STAGE2_BIN): $(STAGE2_ELF)
	$(OBJCOPY) -O binary $< $@

# === DISK IMAGE ===
$(DISK): $(STAGE1_BIN) $(STAGE2_BIN)
	@truncate -s $(DISK_SIZE) $(DISK)
	dd if=$(STAGE1_BIN) of=$(DISK) conv=notrunc
	dd if=$(STAGE2_BIN) of=$(DISK) bs=$(SECTOR) seek=1 conv=notrunc

# === RULARE QEMU ===
run: $(DISK)
	$(QEMU) -drive file=$(DISK),format=raw -no-reboot -no-shutdown -d int,cpu_reset

# === CURĂȚARE ===
clean:
	rm -rf $(BINDIR) $(BUILDDIR) $(DISK)
