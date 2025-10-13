# # Makefile pentru asamblare și rulare în QEMU
# ASM := nasm
# QEMU := qemu-system-i386
# DISK := boot/disk.img

# NASMFLAGS := -f bin -Iboot/includes/ -I.

# STAGE1_SRC := boot/stage1.asm
# STAGE2_SRC := boot/stage2.asm
# STAGE1_BIN := boot/bin/stage1.bin
# STAGE2_BIN := boot/bin/stage2.bin

# all: run

# $(STAGE1_BIN): $(STAGE1_SRC)
# 	$(ASM) $(NASMFLAGS) $< -o $@

# $(STAGE2_BIN): $(STAGE2_SRC)
# 	$(ASM) $(NASMFLAGS) $< -o $@

# $(DISK): $(STAGE1_BIN) $(STAGE2_BIN)
# 	dd if=$(STAGE1_BIN) of=$(DISK) conv=notrunc
# 	dd if=$(STAGE2_BIN) of=$(DISK) bs=512 seek=1 conv=notrunc

# run: $(DISK)
# 	$(QEMU) -drive format=raw,file=$(DISK)

# clean:
# 	rm -f $(STAGE1_BIN) $(STAGE2_BIN) $(DISK)

# --- Build & Run with QEMU ---
ASM   := nasm
QEMU  := qemu-system-i386

BINDIR    := boot/bin
INCDIRS   := -Iboot/includes/ -I.
DISK      := boot/disk.img
DISK_DIR  := $(dir $(DISK))

NASMFLAGS := -f bin $(INCDIRS)

STAGE1_SRC := boot/stage1.asm
STAGE2_SRC := boot/stage2.asm
STAGE1_BIN := $(BINDIR)/stage1.bin
STAGE2_BIN := $(BINDIR)/stage2.bin

.PHONY: all run clean
all: run

DISK_SIZE := 64M      
SECTOR    := 512

$(BINDIR) $(DISK_DIR):
	@mkdir -p $@


$(STAGE1_BIN): $(STAGE1_SRC) | $(BINDIR)
	$(ASM) $(NASMFLAGS) $< -o $@

$(STAGE2_BIN): $(STAGE2_SRC) | $(BINDIR)
	$(ASM) $(NASMFLAGS) $< -o $@

$(DISK): $(STAGE1_BIN) $(STAGE2_BIN) | $(DISK_DIR)
	@truncate -s $(DISK_SIZE) $(DISK)             
	dd if=$(STAGE1_BIN) of=$(DISK) conv=notrunc
	dd if=$(STAGE2_BIN) of=$(DISK) bs=$(SECTOR) seek=1 conv=notrunc

run: $(DISK)
	$(QEMU) -drive file=$(DISK),format=raw

clean:
	rm -f $(STAGE1_BIN) $(STAGE2_BIN) $(DISK)
