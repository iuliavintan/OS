#include"../vga.h"
#include"e820.h"
#include"../stdint.h"


e820_entry_t g_e820_entries[128];
e820_map_t   g_e820_map = { g_e820_entries, 0 };

void e820_import(void){
    uint32_t count = *(volatile uint32_t*)(uintptr_t)E820_COUNT_PHYS;
    if(count>E820_MAX_ENTRIES)
        count=E820_MAX_ENTRIES;

    e820_entry_t *src= (e820_entry_t *)(uintptr_t)E820_ENTRIES_PHYS;

    for(uint32_t i=0; i<count; i++){
        g_e820_entries[i]=src[i];
    }

    g_e820_map.entries=g_e820_entries;
    g_e820_map.count=count;

  //  print("E820: imported %d entries\n", count);
    // for(uint32_t i=0; i<count; i++){
    //     e820_entry_t *e=&g_e820_entries[i];
    //   //  print("[%d] base=0x%x%x len=0x%x%x type=%d attr=%x\n",
    //            i,
    //            (uint32_t)(e->base >> 32), (uint32_t)e->base,
    //            (uint32_t)(e->length >> 32), (uint32_t)e->length,
    //            e->type, e->extraattr);
    // }


}

uint64_t e820_get_usable_ram(void){
    uint64_t total_mem=0;
    for(uint32_t i=0; i<g_e820_map.count; i++){
        e820_entry_t *e= &g_e820_map.entries[i];
        if(e->type==1){
            total_mem+=e->length;
        }   
    }
    return total_mem;
}