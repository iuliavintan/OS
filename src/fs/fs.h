#pragma once

#include "../utils/stdint.h"

int fs_init(void);
int fs_load_file(const char *name8, void *dest, uint32_t max_sectors, uint32_t *out_sectors);
int fs_write_file(const char *name8, const void *src, uint32_t bytes);
int fs_file_capacity(const char *name8, uint32_t *out_bytes);
int fs_ready(void);
void fs_iterate_root(void (*cb)(const char *name8, uint16_t start_cluster, void *ctx), void *ctx);
