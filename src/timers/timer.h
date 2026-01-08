#ifndef TIMER_H
#define TIMER_H
#include"../utils/util.h"
#include"../utils/stdint.h"
#include"../interrupts/idt.h"
#include"../interrupts/irq.h"
#include"../interrupts/isr.h"
#include"../utils/vga.h"

void initTimer();
uint64_t timer_get_ticks(void);
void tick_set(int enabled);
int tick_get(void);

void irq0on( IntrerruptRegisters *regs);
 //runs when the interrupt is trigger associated with the timer => irq0


#endif
