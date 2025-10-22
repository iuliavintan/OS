#ifndef IRQ_H
#define IRQ_H

#include "stdint.h"
#include "../util.h"


void irq_install_handler(int irq, void (*handler)(struct IntrerruptRegisters *r));
void irq_uninstall_handler(int irq);
void irq_handler(struct IntrerruptRegisters *regs);
void irq0_handler(struct IntrerruptRegisters *r);
void irq1_handler(struct IntrerruptRegisters *r);

void idt_enable_keyboard(void);
//extern volatile uint32_t keyboard_irq_count;



#endif