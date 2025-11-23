#pragma once
#include "../utils/util.h"
#include "../utils/vga.h"
#include "../utils/stdint.h"
#include "../utils/stddef.h"
#include "e820.h"

#define PAGE_SIZE 4096u

void pmm_init(void);

/* Single-page API (4 KiB pages) */
uintptr_t pmm_alloc_page(void);
void      pmm_free_page(uintptr_t phys_addr);

/* Stats & helpers */
size_t    pmm_total_pages(void);
size_t    pmm_available_pages(void);

/* Reserve/release arbitrary physical ranges (byte granularity) */
void pmm_reserve_range(uint64_t base, uint64_t length);
void pmm_release_range(uint64_t base, uint64_t length);
