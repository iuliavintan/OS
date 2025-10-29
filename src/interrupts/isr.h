#ifndef ISR_H
#define ISR_H

#include "stdint.h"
#include "../util.h"

void isr_dispatch(struct IntrerruptRegisters* regs);
void common_isr_handler(struct IntrerruptRegisters* regs);
void page_fault_handler(uint32_t err_no, uint32_t cr2_fault_addr);

#endif