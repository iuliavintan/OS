#include"kheap.h"
#include"pmm.h"
#include"../util.h" 
#include"../stdint.h"
#include"vmm.h"

#define KHEAP_START 0x40000000u     //has to be page aligned (orice multiplu de 4096)
#define KHEAP_MAX_SIZE (4*1024*1024u) //4MiB - small heap, easy to control

static uintptr_t heap_start=KHEAP_START;
static uintptr_t heap_end = KHEAP_START;    //"consumed" heap up to this point
static uintptr_t heap_mapped_end = KHEAP_START;         //mapped pages up to this point
static uintptr_t heap_limit = KHEAP_START + KHEAP_MAX_SIZE; 


static inline uint32_t align_up(uint32_t val, uint32_t align){
    return (val + (align - 1)) & ~(align - 1);
}

//heap grows by mapping new pages up untill new_end
static int kheap_grow_to(uintptr_t new_end){
    new_end=align_up((uint32_t)new_end, PAGE_SIZE);


    while(heap_mapped_end < new_end){
        uintptr_t frame = pmm_alloc_page();
        if(frame==0) return -1; //out of physicql memory

        int res= vmm_map_page(heap_mapped_end, frame, VMM_FLAG_PRESENT | VMM_FLAG_RW);

        if(res!=0) return -1; //we cant map -> we should panic

        heap_mapped_end+=PAGE_SIZE;

    }
    return 0;
}

void kheap_init(void){
    heap_start=KHEAP_START;
    heap_end=KHEAP_START;
    heap_mapped_end=KHEAP_START;
    heap_limit=KHEAP_START + KHEAP_MAX_SIZE;
}

void *kmalloc(uint32_t size){
    if(size==0) return 0;

    //allign at 8 bytes
    size=align_up(size, 8);

    if(heap_end + size > heap_limit ){
        return 0;   //out of heap space
    }

    //daca nu avem destule pagini mapate cerem mai multe
    if(heap_end+size>heap_mapped_end){
        if(kheap_grow_to(heap_end+size)!=0){
            return 0;
        }
    }

    void *ptr = (void *)heap_end;
    heap_end+=size;
    return ptr;
}


void kfree(void *ptr){
    (void)ptr;
    //trebe implementata
}


