#ifndef TIMER_H
#define TIMER_H
#include"../util.h"

void initTimer();

void irq0on( IntrerruptRegisters *regs);
 //runs when the interrupt is trigger associated with the timer => irq0


#endif