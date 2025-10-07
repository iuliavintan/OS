# Makefile pentru asamblare și rulare în QEMU

ASM = nasm
QEMU = qemu-system-i386
DISK = disk.img

STAGE1_SRC = stage1.asm
STAGE2_SRC = stage2.asm
STAGE1_BIN = stage1.bin
STAGE2_BIN = stage2.bin

all: run

$(STAGE1_BIN): $(STAGE1_SRC)
	$(ASM) -f bin $< -o $@

$(STAGE2_BIN): $(STAGE2_SRC)
	$(ASM) -f bin $< -o $@

$(DISK): $(STAGE1_BIN) $(STAGE2_BIN)
	dd if=$(STAGE1_BIN) of=$(DISK) conv=notrunc
	dd if=$(STAGE2_BIN) of=$(DISK) bs=512 seek=1 conv=notrunc

run: $(DISK)
	$(QEMU) -drive format=raw,file=$(DISK)

clean:
	rm -f $(STAGE1_BIN) $(STAGE2_BIN) $(DISK)
