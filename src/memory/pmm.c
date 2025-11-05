#include "pmm.h"
// #include "e820.h"
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
    e820_entry_t *entries = g_e820_map.entries;
    for(uint32_t i = 0 ; i < g_e820_map.count - 1; ++i )
    for( uint32_t j = i + 1 ; j < g_e820_map.count; ++j ){
        if( entries[i].base > entries[j].base){
            e820_entry_t tmp = entries[i];
            entries[i] = entries[j];
            entries[j] = tmp;
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
static void dump_chunks(void){
    print("usable_chunks_number=%x\n", (unsigned)usable_chunks_number);
    for (uint64_t i = 0; i < usable_chunks_number; ++i){
        uint64_t s = usable_chunks[i].start, e = usable_chunks[i].end;
        uint64_t pages = (e - s) / PAGE_SIZE;
        print("chunk[%x]: %x .. %x  pages=%x\n", (unsigned)i, (unsigned)s, (unsigned)e, (unsigned)pages);
    }
}

static int validate_chunks(void){
    int errs = 0;
    for (uint64_t i = 0; i < usable_chunks_number; ++i){
        uint64_t s = usable_chunks[i].start, e = usable_chunks[i].end;

        if (s & (PAGE_SIZE-1)) { print("ERR: start not aligned @[%x]\n", (unsigned)i); ++errs; }
        if (e & (PAGE_SIZE-1)) { print("ERR: end not aligned   @[%x]\n", (unsigned)i); ++errs; }
        if (e <= s)            { print("ERR: empty/neg range   @[%x]\n", (unsigned)i); ++errs; }

        if (i > 0){
            uint64_t prev_e = usable_chunks[i-1].end;
            if (s <= prev_e) { // must be strictly greater; we merge touch/overlap already
                print("ERR: overlap/touch after merge between [%x] and [%x]\n", (unsigned)(i-1), (unsigned)i);
                ++errs;
            }
        }
    }
    return errs;
}

static void chunk_chk(){
    dump_chunks();
    int errs = validate_chunks();
    if (errs){
        print("VALIDATION FAILED: %x\n", (unsigned)errs);
        for(;;) { /* halt so you can read output */ }
    } else {
        print("alignment+merge OK\n");
    }
}

static uint64_t highest_phys_addr(){
    uint64_t max_addr = 0;
    for(uint32_t i = 0 ; i < g_e820_map.count; ++i ){
        uint64_t end = g_e820_map.entries[i].base + g_e820_map.entries[i].length;
        if(end > max_addr) max_addr = end;
    }
    return max_addr;
}

void pmm_init(){
    align_pages();
    //chunk_chk();
    uint64_t highest_addr = highest_phys_addr();

}