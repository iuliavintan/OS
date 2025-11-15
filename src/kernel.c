#include"vga.h"
#include"stdint.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "interrupts/isr.h"
#include "util.h"
#include"timers/timer.h"
#include "memory/e820.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

void kmain(void);

void kmain(void)
{
    reset();
    //disable_cursor();
    kprint("[KERNEL] Booting...\n");
    init_idt();
    idt_enable_keyboard();
    initTimer();
    kprint("[KERNEL] PM OK, IDT OK, IRQ0 OK, IRQ1 OK\n");
    e820_import();
    uint64_t total_ram = e820_get_usable_ram();
    print("Total usable RAM: %d MB\n", (uint32_t)(total_ram / (1024*1024)));

    pmm_init();
    kprint("[PMM] after init: free=%d pages (approx %d MB)\n",(uint32_t)pmm_available_pages(), (uint32_t)((pmm_available_pages() * PAGE_SIZE) / (1024*1024)));
    
    vmm_init();
    kprint("[VMM] initialized\n");

    //test1 should print 0x000B8000
    uintptr_t phys;
     if (vmm_resolve_page(0x000B8000, &phys) == 0) {
          kprint("VMM: VA 0x000B8000 -> PA %p\n", (void*)phys);
     } 
     else {
          kprint("VMM: resolve failed for VGA\n");
     }
     //test2
     uintptr_t test_va  = 0x00400000;      // 4 MiB
     uintptr_t test_pa  = pmm_alloc_page(); // ia un cadru fizic
     kprint("VMM: test_pa = %p\n", (void*)test_pa);

     if (vmm_map_page(test_va, test_pa, VMM_FLAG_PRESENT | VMM_FLAG_RW) == 0) {
          kprint("VMM: mapped VA %p -> PA %p\n", (void*)test_va, (void*)test_pa);

          uint32_t *p = (uint32_t*)test_va;
          p[0] = 0xDEADBEEF;

          uintptr_t resolved;
          if (vmm_resolve_page(test_va, &resolved) == 0) {
               kprint("VMM: resolved %p -> %p, value=%x\n",
                         (void*)test_va, (void*)resolved, p[0]);
          } else {
               kprint("VMM: resolve failed for test_va\n");
          }
     } 
     else {
          kprint("VMM: map_page failed\n");
     }
     
     //test3
     if (vmm_unmap_page(test_va) == 0) {
          kprint("VMM: unmapped test_va\n");
     } 
     else {
          kprint("VMM: unmap failed for test_va\n");
     }

     uintptr_t resolved2;
     if (vmm_resolve_page(test_va, &resolved2) == 0) {
          kprint("VMM: still mapped?! %p -> %p\n", (void*)test_va, (void*)resolved2);
     } 
     else {
          kprint("VMM: resolve correctly fails after unmap\n");
     }

    uint8_t mask = InPortByte(0x21);
    mask &= ~(1 << 0); // dezactivează masca pentru IRQ0 (timer)
    OutPortByte(0x21, mask);
    asm volatile("sti");

     for (;;)
        asm volatile("hlt");
}
