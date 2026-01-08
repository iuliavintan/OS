#include"utils/vga.h"
#include"utils/stdint.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "interrupts/isr.h"
#include "utils/util.h"
#include"timers/timer.h"
#include "memory/e820.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/kheap.h"
#include "memory/tests.h"
#include "fs/fs.h"

// #define MEM_TESTS
// #define KHEAP_TESTS
// #define HEAP_DUMP
// #define FILE_SYS_TEST

void kmain(void);

static void taskA(void) {
    for (;;) {
        putc('A');
        for (volatile uint32_t i = 0; i < 2000000; i++) { }
    }
}

static void taskB(void) {
    for (;;) {
        putc('B');
        for (volatile uint32_t i = 0; i < 2000000; i++) { }
    }
}

void kmain(void)
{
     reset();
     //disable_cursor();
     // kprint("[KERNEL] Booting...\n");
     kprint("[KERNEL] ");
     print("Booting...\n");
    
     init_idt();
     idt_enable_keyboard();
     initTimer();
     kprint("[KERNEL] ");
     print("PM OK, IDT OK, IRQ0 OK, IRQ1 OK\n");
     e820_import();
     uint64_t total_ram = e820_get_usable_ram();
     kprint("[KERNEL] ");
     print("Total usable RAM: %d MB\n", (uint32_t)(total_ram / (1024*1024)));

     pmm_init();
     kprint("[KERNEL] ");
     print("PMM after init: free=%d pages (approx %d MB)\n",(uint32_t)pmm_available_pages(), (uint32_t)((pmm_available_pages() * PAGE_SIZE) / (1024*1024)));
     
     vmm_init();
     kprint("[KERNEL] ");
     print("VMM initialized\n");

     kheap_init();

     kprint("[KERNEL] ");
     print("Heap initialized\n");

     #ifdef MEM_TESTS
     run_memory_smoke_test();
     #endif
     #ifdef KHEAP_TESTS
     kheap_test_with_logging();
     #endif
     #ifdef HEAP_DUMP
     heap_dump();
     #endif
     #ifdef FILE_SYS_TEST
    if (fs_init() == 0) {
            uint32_t sectors = 0;
            if (fs_load_file("CALC    ", (void *)0x300000, 128, &sectors) == 0) {
                kprint("[KERNEL] ");
                print("FS loaded CALC (%d sectors)\n", sectors);
                void (*calc_entry)(void) = (void (*)(void))0x300000;
                calc_entry();
            } else {
                kprint("[KERNEL] ");
                print("FS load failed\n");
            }
        } else {
            kprint("[KERNEL] ");
            print("FS init failed\n");
        }

     #endif

     uint8_t mask = InPortByte(0x21);
     mask &= ~(1 << 0); // dezactivează masca pentru IRQ0 (timer)
     OutPortByte(0x21, mask);
     asm volatile("sti");

     sched_init(5);             // 5 ticks quantum (~50ms if 100Hz PIT)
    task_create(taskA, 0);
    task_create(taskB, 0);
    sched_enable(1);
        
     for (;;)
        asm volatile("hlt");
}
