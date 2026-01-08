# ===================== Toolchain =====================
ASM      ?= nasm
CC       ?= gcc
HOSTCC   ?= gcc
LD       ?= ld
OBJCOPY  ?= objcopy
QEMU     ?= qemu-system-i386

LDSCRIPT := boot/link.ld     # stage2/linker (expects pm_entry)

# ===================== Dirs & Files =====================
BINDIR    := boot/bin
BUILDDIR  := build
DISK      := boot/disk.img
DISK_SIZE := 128M
SECTOR    := 512

# ---- fixed layout: stage2 = exactly 5 sectors; kernel starts after it ----
STAGE2_SECT := 5

# Include paths
C_INCLUDES     := -Iboot/includes -Isrc -Iutils -I.
NASM_INCLUDES  := -Iboot/includes/ -Isrc/ -I.

# Sources
STAGE1_SRC := boot/stage1.asm

# Split stage2 (only files in boot/, except stage1) and kernel (everything in src/)
STAGE2_C_SRCS   := $(shell find boot -maxdepth 1 -type f -name '*.c')
STAGE2_ASM_SRCS := $(filter-out boot/stage1.asm,$(shell find boot -maxdepth 1 -type f -name '*.asm'))
STAGE2_C_OBJS   := $(STAGE2_C_SRCS:%.c=$(BUILDDIR)/%.o)
STAGE2_ASM_OBJS := $(STAGE2_ASM_SRCS:%.asm=$(BUILDDIR)/%.asm.o)
STAGE2_OBJS     := $(STAGE2_C_OBJS) $(STAGE2_ASM_OBJS)

# KERNEL_C_SRCS   := $(shell find src -type f -name '*.c')
# KERNEL_ASM_SRCS := $(shell find src -type f -name '*.asm')
# KERNEL_OBJS     := $(KERNEL_C_SRCS:%.c=$(BUILDDIR)/%.o) \
#                    $(KERNEL_ASM_SRCS:%.asm=$(BUILDDIR)/%.o)

# Outputs
STAGE1_BIN := $(BINDIR)/stage1.bin
STAGE2_ELF := $(BUILDDIR)/stage2.elf
STAGE2_BIN := $(BINDIR)/stage2.bin
STAGE2_PAD := $(BINDIR)/stage2.padded.bin
KERNEL_ELF := $(BUILDDIR)/kernel.elf
KERNEL_BIN := $(BINDIR)/kernel.bin
FS_TABLE   := $(BINDIR)/fs.bin
MKFS_TOOL  := $(BUILDDIR)/utils/mkfs
CALC_ELF   := $(BUILDDIR)/calc.elf
CALC_BIN   := $(BINDIR)/calc.bin

# ===================== Flags =====================
CFLAGS   := -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -O2 -Wall -Wextra -mno-sse -mno-sse2 -mno-mmx -msoft-float -fno-asynchronous-unwind-tables $(C_INCLUDES)
CFLAGS   += -MMD -MP
HOSTCFLAGS := -O2 -Wall -Wextra
USER_CFLAGS := -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -O2 -Wall -Wextra -nostdlib -nostartfiles -Iutils -I.
LDFLAGS  := -m elf_i386 -T $(LDSCRIPT)

# Kernel link flags (avoid pm_entry warning); adjust to your kernel linker later.
KERNEL_LDFLAGS := -m elf_i386 -e kmain

# ===================== Phony =====================
.PHONY: all run clean dirs
all: run

# ===================== Build rules =====================
dirs:
	@mkdir -p $(BINDIR) $(BUILDDIR)

# Stage1 (raw 512-byte bin)
$(STAGE1_BIN): $(STAGE1_SRC) | dirs
	$(ASM) -f bin $(NASM_INCLUDES) $< -o $@

# Generic compilers
$(BUILDDIR)/%.o: %.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.asm.o: %.asm | dirs
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $(NASM_INCLUDES) $< -o $@

# Stage2 (ELF -> BIN)
# $(STAGE2_ELF): $(STAGE2_OBJS) | dirs
# 	$(LD) $(LDFLAGS) -o $@ $^

# $(STAGE2_BIN): $(STAGE2_ELF)
# 	$(OBJCOPY) -O binary $< $@


$(STAGE2_ELF): $(STAGE2_OBJS) | dirs
	$(LD) -m elf_i386 -T boot/link.ld -o $@ $^

$(STAGE2_BIN): $(STAGE2_ELF)
	$(OBJCOPY) -O binary $< $@

# Pad Stage2 to EXACTLY 3 sectors (3 * 512 = 1536 B)
$(STAGE2_PAD): $(STAGE2_BIN) | dirs
	@sz=$$(stat -c%s "$<") || { echo "stat failed on $<"; exit 1; }; \
	max=$$(( $(STAGE2_SECT) * $(SECTOR) )); \
	if [ $$sz -gt $$max ]; then \
	  echo "ERROR: stage2 ($$sz B) exceeds $(STAGE2_SECT) sectors ($$max B)"; exit 1; \
	fi; \
	cp "$<" "$@"; \
	pad=$$(( $$max - $$sz )); \
	dd if=/dev/zero bs=1 count=$$pad >> "$@" 2>/dev/null; \
	echo "stage2 padded to $$max bytes"

# Kernel (ELF -> BIN). Use a separate entry to avoid pm_entry warning.
# Kernel sources (place kernel.asm and kernel C under src/ or kernel/)
KERNEL_ASM_SRCS := $(shell find src -type f -name '*.asm' -o -name 'kernel.asm')
UTILS_C_SRCS    := $(shell find utils -type f -name '*.c' ! -name 'mkfs.c')
KERNEL_C_SRCS   := $(shell find src -type f -name '*.c') $(UTILS_C_SRCS)
KERNEL_ASM_OBJS := $(KERNEL_ASM_SRCS:%.asm=$(BUILDDIR)/%.asm.o)
KERNEL_C_OBJS   := $(KERNEL_C_SRCS:%.c=$(BUILDDIR)/%.o)
KERNEL_OBJS     := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

KERNEL_ELF := $(BUILDDIR)/kernel.elf
KERNEL_BIN := $(BINDIR)/kernel.bin

$(KERNEL_ELF): $(KERNEL_OBJS) boot/kernel.ld | dirs
	$(LD) -m elf_i386 -T boot/kernel.ld -o $@ $(filter-out boot/kernel.ld,$^)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# User program (calc)
$(BUILDDIR)/src/user/%.o: src/user/%.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(CALC_ELF): $(BUILDDIR)/src/user/calc.o | dirs
	$(LD) -m elf_i386 -Ttext 0x300000 -e calc_main -o $@ $^

$(CALC_BIN): $(CALC_ELF)
	$(OBJCOPY) -O binary $< $@

# Host tool to build filesystem table
$(MKFS_TOOL): utils/mkfs.c | dirs
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

# Filesystem tables (3 sectors starting at LBA1)
$(FS_TABLE): $(STAGE2_PAD) $(KERNEL_BIN) $(CALC_BIN) $(MKFS_TOOL) | dirs
	@$(MKFS_TOOL) --out "$@" --stage2 "$(STAGE2_PAD)" --kernel "$(KERNEL_BIN)" --prog "$(CALC_BIN)"

# Disk image: [LBA0: stage1][LBA1: fs hdr][LBA2: FAT][LBA3: root][LBA4..: data]
$(DISK): $(STAGE1_BIN) $(STAGE2_PAD) $(KERNEL_BIN) $(CALC_BIN) $(FS_TABLE)
	@truncate -s $(DISK_SIZE) "$(DISK)"
	dd if="$(STAGE1_BIN)" of="$(DISK)" bs=$(SECTOR) seek=0 conv=notrunc
	dd if="$(FS_TABLE)" of="$(DISK)" bs=$(SECTOR) seek=1 conv=notrunc
	dd if="$(STAGE2_PAD)" of="$(DISK)" bs=$(SECTOR) seek=4 conv=notrunc
	@lba=$$(( 4 + $(STAGE2_SECT) )); \
	echo "Writing kernel at LBA $$lba"; \
	dd if="$(KERNEL_BIN)" of="$(DISK)" bs=$(SECTOR) seek=$$lba conv=notrunc
	@plba=$$(( 4 + $(STAGE2_SECT) + ($$(stat -c%s "$(KERNEL_BIN)") + 511) / 512 )); \
	echo "Writing calc at LBA $$plba"; \
	dd if="$(CALC_BIN)" of="$(DISK)" bs=$(SECTOR) seek=$$plba conv=notrunc

run: $(DISK)
	$(QEMU) -drive file=$(DISK),format=raw -no-shutdown -d int,cpu_reset -m 4G

clean:
	rm -rf $(BINDIR) $(BUILDDIR) $(DISK)

# Auto-deps
-include $(C_OBJS:.o=.d)
