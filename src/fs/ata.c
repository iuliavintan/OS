#include "ata.h"
#include "../utils/util.h"

#define ATA_PRIMARY_IO     0x1F0
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

#define ATA_CMD_READ_SECTORS 0x20
#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

static int ata_wait_not_busy(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if ((InPortByte(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY) == 0) {
            return 0;
        }
    }
    return -1;
}

static int ata_wait_drq(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        uint8_t st = InPortByte(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (st & ATA_SR_ERR) {
            return -1;
        }
        if ((st & ATA_SR_BSY) == 0 && (st & ATA_SR_DRQ)) {
            return 0;
        }
    }
    return -1;
}

int ata_pio_read(uint32_t lba, uint8_t count, void *buf) {
    if (count == 0) {
        return 0;
    }
    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    OutPortByte(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    OutPortByte(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, count);
    OutPortByte(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    OutPortByte(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    OutPortByte(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    OutPortByte(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);

    uint8_t *dst = (uint8_t *)buf;
    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_drq() != 0) {
            return -1;
        }
        for (uint16_t i = 0; i < 256; i++) {
            uint16_t data = InPortWord(ATA_PRIMARY_IO + ATA_REG_DATA);
            *dst++ = (uint8_t)(data & 0xFF);
            *dst++ = (uint8_t)((data >> 8) & 0xFF);
        }
    }
    return 0;
}
