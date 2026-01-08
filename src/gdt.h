#ifndef GDT_H
#define GDT_H

#include "utils/stdint.h"

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x1B
#define USER_DS   0x23

void gdt_init(void);
void tss_set_kernel_stack(uint32_t esp0);

#endif
