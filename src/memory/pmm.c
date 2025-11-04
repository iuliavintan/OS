#include "pmm.h"

typedef struct range_t{
    uint64_t start,end;
}range_t;

static range_t usable_chunks[128];
static uint64_t usable_chunks_number = 0;

static inline uint64_t align_down(uint64_t x, uint64_t page_size) {
    return x & ~(page_size - 1);
}
static inline uint64_t align_up(uint64_t x, uint64_t page_size) {
    return (x + (page_size - 1)) & ~(page_size - 1);
}

static void sort_e820(){
    for(uint32_t i = 0 ; i < g_e820_map.count - 1; ++i )
    for( uint32_t j = i + 1 ; j < g_e820_map.count; ++j ){
        if( g_e820_entries[i].base > g_e820_entries[j].base){
            e820_entry_t tmp = g_e820_entries[i];
            g_e820_entries[i] = g_e820_entries[j];
            g_e820_entries[j] = tmp;
        }  
    }
}

static void align_pages(){
    e820_import();   
    sort_e820();
    uint32_t entry_count = g_e820_map.count;
    range_t prev;

    usable_chunks_number = 0;
    for(uint32_t i = 0 ; i < entry_count ; ++i ){
        if( g_e820_entries[i].type == 1 ){
            uint64_t end = g_e820_entries[i].base + g_e820_entries[i].length;
            uint64_t base = g_e820_entries[i].base;
            uint64_t s = align_up(base,PAGE_SIZE);
            uint64_t e = align_down(end,PAGE_SIZE);

            if( e <= s ){
                continue;
            }
            else{
                if( usable_chunks_number == 0 ){
                    prev.start = s;
                    prev.end = e;
                    usable_chunks[0].start = s;
                    usable_chunks[0].end = e;
                    usable_chunks_number = 1;
                }
                else{
                    if( prev.end >= s ){
                        uint64_t *last_end = &usable_chunks[usable_chunks_number - 1].end;
                        if(e > *last_end) *last_end = e;
                        prev.end = *last_end;
                    } 
                    else{
                        usable_chunks[usable_chunks_number].start = s;
                        usable_chunks[usable_chunks_number].end = e;
                        usable_chunks_number++;

                    }
                    prev.start = s;
                    prev.end = e;
                }
                
            }
        }
    }

}

void pmm_init(){
    align_pages();
}