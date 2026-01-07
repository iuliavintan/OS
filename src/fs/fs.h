#pragma once

#include "../utils/stdint.h"

int fs_init(void);
int fs_load_file(const char *name8, void *dest, uint32_t max_sectors, uint32_t *out_sectors);
