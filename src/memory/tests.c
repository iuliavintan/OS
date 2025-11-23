#include "tests.h"

void run_memory_smoke_test(void){
    void* a = kmalloc(16);
    void* b = kmalloc(32);
    void* c = kmalloc(1024);

    print("a = %x\n", a);
    print("b = %x\n", b);
    print("c = %x\n", c);

    //test1 should print 0x000B8000
    uintptr_t phys;
    if (vmm_resolve_page(0x000B8000, &phys) == 0) {
        print("VMM: VA 0x000B8000 -> PA %x\n", (void*)phys);
    } 
    else {
        print("VMM: resolve failed for VGA\n");
    }

    //test2
    uintptr_t test_va  = 0x00400000;      // 4 MiB
    uintptr_t test_pa  = pmm_alloc_page(); // ia un cadru fizic
    print("VMM: test_pa = %x\n", (void*)test_pa);

    if (vmm_map_page(test_va, test_pa, VMM_FLAG_PRESENT | VMM_FLAG_RW) == 0) {
        print("VMM: mapped VA %x -> PA %x\n", (void*)test_va, (void*)test_pa);

        uint32_t *p = (uint32_t*)test_va;
        p[0] = 0xDEADBEEF;

        uintptr_t resolved;
        if (vmm_resolve_page(test_va, &resolved) == 0) {
            print("VMM: resolved %x -> %x, value=%x\n",
                        (void*)test_va, (void*)resolved, p[0]);
        } else {
            print("VMM: resolve failed for test_va\n");
        }
    } 
    else {
        print("VMM: map_page failed\n");
    }
    
    //test3
    if (vmm_unmap_page(test_va) == 0) {
        print("VMM: unmapped test_va\n");
    } 
    else {
        print("VMM: unmap failed for test_va\n");
    }

    uintptr_t resolved2;
    if (vmm_resolve_page(test_va, &resolved2) == 0) {
        print("VMM: still mapped?! %x -> %x\n", (void*)test_va, (void*)resolved2);
    } 
    else {
        print("VMM: resolve correctly fails after unmap\n");
    }
    volatile uint32_t *q = (uint32_t*)test_va;
    (void)*q;     // ar trebui să dea #PF după unmap
}

