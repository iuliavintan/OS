#pragma once

#include "../utils/stdint.h"

int ata_pio_read(uint32_t lba, uint8_t count, void *buf);
int ata_pio_write(uint32_t lba, uint8_t count, const void *buf);
