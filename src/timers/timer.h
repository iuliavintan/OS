#ifndef TIMER_H
#define TIMER_H
#include"../utils/util.h"
#include"../utils/stdint.h"
#include"../interrupts/idt.h"
#include"../interrupts/irq.h"
#include"../interrupts/isr.h"
#include"../utils/vga.h"

void initTimer();

void irq0on( IntrerruptRegisters *regs);
 //runs when the interrupt is trigger associated with the timer => irq0


#endif