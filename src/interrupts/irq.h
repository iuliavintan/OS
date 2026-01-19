#ifndef IRQ_H
#define IRQ_H

#include "../utils/stdint.h"
#include "../utils/util.h"
#include "../utils/vga.h"

void irq_install_handler(int irq, void (*handler)(struct IntrerruptRegisters *r));
void irq_uninstall_handler(int irq);
uint32_t irq_handler(struct IntrerruptRegisters *regs);
void irq0_handler(struct IntrerruptRegisters *r);
void irq1_handler(struct IntrerruptRegisters *r);

void idt_enable_keyboard(void);
int kbd_getchar(void);

#define KBD_KEY_UP 0x80
#define KBD_KEY_DOWN 0x81
#define KBD_KEY_RIGHT 0x82
#define KBD_KEY_LEFT 0x83
//extern volatile uint32_t keyboard_irq_count;



#endif
