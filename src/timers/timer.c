#include"../stdint.h"
#include"../util.h"
#include"../interrupts/idt.h"
#include"../interrupts/irq.h"
#include"../interrupts/isr.h"
#include"../vga.h"
#include"timer.h"

uint64_t ticks;
const uint32_t frequency = 1000;

void irq0on(IntrerruptRegisters *regs)
{
    ticks+=1;
    // print("Timer ticked!");
}

void initTimer()
{
    ticks=0;
    irq_install_handler(0, &irq0on); 

    //1.1931816666 MHz
    uint32_t divisor = 1193180/frequency; 
    
    // 0x43 mode/command register
    //0011 0110 => 16-bit binary, square wave generator, acces mode:lobyte/hibyte, channel 0
    OutPortByte(0x43, 0x36);

    //0x40 - channel port for channel 0
    OutPortByte(0x40, (uint8_t)(divisor & 0xFF));
    OutPortByte(0x40, (uint8_t)((divisor >> 8 ) & 0xFF));

}


