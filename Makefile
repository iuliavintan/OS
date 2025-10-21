# # === CONFIGURARE GENERALĂ ===
# ASM      := nasm
# CC       := gcc
# LD       := ld
# OBJCOPY  := objcopy

# # CC       := i686-elf-gcc      # prefer cross toolchain
# # LD       := i686-elf-ld
# # OBJCOPY  := i686-elf-objcopy


# QEMU     := qemu-system-i386
# LDSCRIPT := boot/link.ld

# # === DIRECTOARE ===
# BINDIR   := boot/bin
# BUILDDIR := build
# DISK     := boot/disk.img
# DISK_SIZE := 64M
# SECTOR    := 512

# INCLUDES := -Iboot/includes/ -Isrc/ -I. 

# # === SURSE ===
# STAGE1_SRC := boot/stage1.asm
# STAGE2_ASM := boot/stage2.asm
# STAGE2_C   := boot/stage2.c
# KERNEL_C   := src/kernel.c
# KERNEL_OBJ := $(BUILDDIR)/kernel.o
# C_SRCS := $(foreach d,$(C_SRC_DIRS),$(wildcard $(d)/*.c))
# C_OBJS := $(C_SRCS:%=$(BUILDDIR)/%.o)


# # === OUTPUT ===
# STAGE1_BIN := $(BINDIR)/stage1.bin
# STAGE2_ASM_OBJ := $(BUILDDIR)/stage2_asm.o
# STAGE2_C_OBJ   := $(BUILDDIR)/stage2_c.o
# STAGE2_ELF     := $(BUILDDIR)/stage2.elf
# STAGE2_BIN     := $(BINDIR)/stage2.bin

# # === FLAGS ===
# NASMFLAGS := -f elf32 $(INCLUDES)
# CFLAGS    := -m32 -ffreestanding -fno-builtin -nostdlib -nostartfiles -fno-pic -fno-pie -fno-stack-protector -O2 -Wall -Wextra $(INCLUDES)
# LDFLAGS   := -m elf_i386 -T $(LDSCRIPT)
# # === PHONY TARGETS ===
# .PHONY: all run clean dirs

# # === REGULA PRINCIPALĂ ===
# all: run

# # === CREARE DIRECTOARE ===
# dirs:
# 	@mkdir -p $(BINDIR) $(BUILDDIR)

# # === COMPILARE ===
# $(STAGE1_BIN): $(STAGE1_SRC) | dirs
# 	$(ASM) -f bin $(INCLUDES) $< -o $@

# $(STAGE2_ASM_OBJ): $(STAGE2_ASM) | dirs
# 	$(ASM) $(NASMFLAGS) $< -o $@

# # $(STAGE2_C_OBJ): $(STAGE2_C) | dirs
# # 	$(CC) $(CFLAGS) -c $< -o $@

# # $(KERNEL_OBJ): $(KERNEL_C) | dirs
# # 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILDDIR)/%.c.o: %.c
# 	@mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(STAGE2_ELF): $(STAGE2_ASM_OBJ) $(C_OBJS) | dirs
# 	$(LD) $(LDFLAGS) $^ -o $@


# $(STAGE2_BIN): $(STAGE2_ELF)
# 	$(OBJCOPY) -O binary $< $@

# # === DISK IMAGE ===
# $(DISK): $(STAGE1_BIN) $(STAGE2_BIN)
# 	@truncate -s $(DISK_SIZE) $(DISK)
# 	dd if=$(STAGE1_BIN) of=$(DISK) conv=notrunc
# 	dd if=$(STAGE2_BIN) of=$(DISK) bs=$(SECTOR) seek=1 conv=notrunc

# # === RULARE QEMU ===
# run: $(DISK)
# 	$(QEMU) -drive file=$(DISK),format=raw -no-reboot -no-shutdown -d int,cpu_reset

# # === CURĂȚARE ===
# clean:
# 	rm -rf $(BINDIR) $(BUILDDIR) $(DISK)






# ===================== Toolchain =====================
# Prefer the cross toolchain; fall back to host if you must
ASM      ?=nasm
CC       ?=gcc
LD       ?=ld
OBJCOPY  ?=objcopy
QEMU     ?= qemu-system-i386

LDSCRIPT := boot/link.ld     # must have: ENTRY(pm_entry)

# ===================== Dirs & Files =====================
BINDIR    := boot/bin
BUILDDIR  := build
DISK      := boot/disk.img
DISK_SIZE := 64M
SECTOR    := 512

# Include paths
C_INCLUDES     := -Iboot/includes -Isrc -I.
NASM_INCLUDES  := -Iboot/includes/ -Isrc/ -I.

# Sources
STAGE1_SRC := boot/stage1.asm

# Which folders contain your C sources
C_SRC_DIRS := src boot/includes boot

# Find all .c files (recursive)
C_SRCS := $(shell find $(C_SRC_DIRS) -type f -name '*.c')

# Map "path/file.c" -> "build/path/file.o"
C_OBJS := $(C_SRCS:%.c=$(BUILDDIR)/%.o)


# collect all ELF asm sources (except stage1)
#ASM_ELF_SRCS := $(filter-out boot/stage1.asm,$(shell find boot -type f -name '*.asm'))
# collect all ELF asm sources (except stage1)
ASM_ELF_SRCS := $(filter-out boot/stage1.asm,$(shell find boot src -type f -name '*.asm'))


# map "boot/foo/bar.asm" -> "build/boot/foo/bar.o"
ASM_ELF_OBJS := $(ASM_ELF_SRCS:%.asm=$(BUILDDIR)/%.o)


# Outputs
STAGE1_BIN := $(BINDIR)/stage1.bin
STAGE2_ELF := $(BUILDDIR)/stage2.elf
STAGE2_BIN := $(BINDIR)/stage2.bin

# ===================== Flags =====================
CFLAGS   := -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -O2 -Wall -Wextra -mno-sse -mno-sse2 -mno-mmx -msoft-float -fno-asynchronous-unwind-tables $(C_INCLUDES)
CFLAGS   += -MMD -MP                        # auto header deps
LDFLAGS  := -m elf_i386 -T $(LDSCRIPT)      # linker script controls layout

# ===================== Phony =====================
.PHONY: all run clean dirs

all: run

# ===================== Build rules =====================
dirs:
	@mkdir -p $(BINDIR) $(BUILDDIR)

# stage1: raw BIN
$(STAGE1_BIN): $(STAGE1_SRC) | dirs
	$(ASM) -f bin $(NASM_INCLUDES) $< -o $@

# Compile ANY .c into build/<same path>.o
$(BUILDDIR)/%.o: %.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Optional: auto header deps
CFLAGS += -MMD -MP
-include $(C_OBJS:.o=.d)


# any .asm (ELF) -> build/<path>.o (exclude stage1 via ASM_ELF_SRCS/OBJS)
$(BUILDDIR)/%.o: %.asm | dirs
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $(NASM_INCLUDES) $< -o $@

# link all objs into stage2.elf
$(STAGE2_ELF): $(ASM_ELF_OBJS) $(C_OBJS) | dirs
	$(LD) $(LDFLAGS) -o $@ $^

# objcopy to flat binary for writing to disk
$(STAGE2_BIN): $(STAGE2_ELF)
	$(OBJCOPY) -O binary $< $@

# disk image: 64 MB, stage1 at LBA0, stage2 at LBA1+
$(DISK): $(STAGE1_BIN) $(STAGE2_BIN)
	@truncate -s $(DISK_SIZE) $(DISK)
	dd if=$(STAGE1_BIN) of=$(DISK) conv=notrunc
	dd if=$(STAGE2_BIN) of=$(DISK) bs=$(SECTOR) seek=1 conv=notrunc

run: $(DISK)
	$(QEMU) -drive file=$(DISK),format=raw -no-reboot -no-shutdown -d int,cpu_reset

clean:
	rm -rf $(BINDIR) $(BUILDDIR) $(DISK)

# include generated .d dependency files (safe if none exist)
-include $(C_OBJS:.o=.d)

