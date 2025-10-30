#include "stdint.h"

typedef struct{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t extraattr;
}__attribute__((packed)) e820_entry_t;

typedef struct{
    e820_entry_t *entries;
    uint32_t count;
}e820_map_t;

#define E820_COUNT_PHYS 0x00009000u
#define E820_ENTRIES_PHYS 0x00009004u

void e820_import(void);