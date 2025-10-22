#ifndef ISR_H
#define ISR_H

#include "stdint.h"
#include "../util.h"

void isr_handler(struct IntrerruptRegisters* regs);

#endif