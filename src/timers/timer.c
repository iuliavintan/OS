#include"timer.h"

uint64_t ticks;
const uint32_t frequency = 100;
static int g_tick_enabled = 0;

void irq0on(IntrerruptRegisters *regs)
{
    (void)regs;
    ticks+=1;
    if (g_tick_enabled && (ticks % 10 == 0)) {
        print("[TICK] %d\n", (uint32_t)ticks);
    }
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

uint64_t timer_get_ticks(void) {
    return ticks;
}

void tick_set(int enabled) {
    g_tick_enabled = enabled ? 1 : 0;
}

int tick_get(void) {
    return g_tick_enabled;
}
