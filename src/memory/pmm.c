#include "pmm.h"
#include "../util.h"
#include "../vga.h"

extern uint8_t kernel_start, kernel_end;
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

/* --- Bitmap state (added in addition to chunk logic) --- */
static uint8_t *bitmap = 0;      /* 1 bit per page: 1=used, 0=free */
static size_t   bitmap_bytes = 0;
static size_t   total_pages = 0;
static size_t   free_pages  = 0;

static inline void bit_set(size_t i)   { bitmap[i >> 3] |=  (1u << (i & 7)); }
static inline void bit_clr(size_t i)   { bitmap[i >> 3] &= ~(1u << (i & 7)); }
static inline int  bit_tst(size_t i)   { return (bitmap[i >> 3] >> (i & 7)) & 1u; }

static void reserve_range_bitmap(uint64_t base, uint64_t length){
    uint64_t s = align_down(base, PAGE_SIZE);
    uint64_t e = align_up(base + length, PAGE_SIZE);
    if(e <= s) return;
    size_t first = (size_t)(s / PAGE_SIZE);
    size_t last  = (size_t)(e / PAGE_SIZE);
    if(last > total_pages) last = total_pages;
    for(size_t i=first; i<last; ++i){ if(!bit_tst(i)){ bit_set(i); if(free_pages) free_pages--; } }
}

static void release_range_bitmap(uint64_t base, uint64_t length){
    uint64_t s = align_up(base, PAGE_SIZE);
    uint64_t e = align_down(base + length, PAGE_SIZE);
    if(e <= s) return;
    size_t first = (size_t)(s / PAGE_SIZE);
    size_t last  = (size_t)(e / PAGE_SIZE);
    if(last > total_pages) last = total_pages;
    for(size_t i=first; i<last; ++i){ if(bit_tst(i)){ bit_clr(i); free_pages++; } }
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
    /* Build your existing merged, page-aligned usable chunks */
    align_pages();
    //chunk_chk();

    /* Determine total pages and place bitmap right after kernel */
    uint64_t highest_addr = highest_phys_addr();
    total_pages = (size_t)(align_up(highest_addr, PAGE_SIZE) / PAGE_SIZE);

    uintptr_t k_end = (uintptr_t)&kernel_end;
    uintptr_t bm_base = (uintptr_t)align_up((uint64_t)k_end, 0x1000);

    bitmap_bytes = (total_pages + 7) / 8;
    bitmap = (uint8_t*)bm_base;
    memset(bitmap, 0xFF, bitmap_bytes); /* mark all used */
    free_pages = 0;

    /* Free pages based on your usable chunks */
    for(uint64_t i=0; i<usable_chunks_number; ++i){
        uint64_t s = usable_chunks[i].start;
        uint64_t e = usable_chunks[i].end;
        release_range_bitmap(s, e - s);
    }

    /* Reserve low memory, kernel, and bitmap storage */
    reserve_range_bitmap(0, 0x00100000ull);
    uintptr_t k_start = (uintptr_t)&kernel_start;
    reserve_range_bitmap(k_start, (uintptr_t)&kernel_end - k_start);
    reserve_range_bitmap((uint64_t)bm_base, (uint64_t)bitmap_bytes);

    print("[PMM] total=%d pages, free=%d, bitmap=%dB at %p, chunks=%d\n",
          (uint32_t)total_pages, (uint32_t)free_pages, (uint32_t)bitmap_bytes, (void*)bitmap,
          (uint32_t)usable_chunks_number);
}

uintptr_t pmm_alloc_page(void){
    if(free_pages == 0) return 0;
    for(size_t b=0; b<bitmap_bytes; ++b){
        uint8_t v = bitmap[b];
        if(v == 0xFF) continue; /* this byte is full */
        for(int bit=0; bit<8; ++bit){
            if(!(v & (1u<<bit))){
                size_t idx = (b<<3) | (size_t)bit;
                if(idx >= total_pages) break;
                bit_set(idx);
                if(free_pages) free_pages--;
                return (uintptr_t)(idx * PAGE_SIZE);
            }
        }
    }
    return 0;
}

void pmm_free_page(uintptr_t phys_addr){
    if(phys_addr & (PAGE_SIZE-1)) return; /* require page aligned */
    size_t idx = (size_t)(phys_addr / PAGE_SIZE);
    if(idx >= total_pages) return;
    if(bit_tst(idx)) { bit_clr(idx); free_pages++; }
}

size_t pmm_total_pages(void){ return total_pages; }
size_t pmm_available_pages(void){ return free_pages; }

void pmm_reserve_range(uint64_t base, uint64_t length){ reserve_range_bitmap(base, length); }
void pmm_release_range(uint64_t base, uint64_t length){ release_range_bitmap(base, length); }
