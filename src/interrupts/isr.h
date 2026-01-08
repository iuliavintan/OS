#ifndef ISR_H
#define ISR_H

#include "../utils/stdint.h"
#include "../utils/util.h"
#include "../utils/vga.h"

uint32_t isr_dispatch(struct IntrerruptRegisters* regs);
void common_isr_handler(struct IntrerruptRegisters* regs);
void page_fault_handler(uint32_t err_no, uint32_t cr2_fault_addr);

#endif
