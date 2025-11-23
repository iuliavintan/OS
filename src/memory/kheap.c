#include"kheap.h"
#include"pmm.h"
#include"../util.h" 
#include"../stdint.h"
#include"vmm.h"

#define KHEAP_START 0x40000000u     //has to be page aligned (orice multiplu de 4096)
#define KHEAP_MAX_SIZE (4*1024*1024u) //4MiB - small heap, easy to control
#define MIN_SPLIT_SIZE 8u           // smallest payload we keep when splitting a block

static uintptr_t heap_start=KHEAP_START;
static uintptr_t heap_end = KHEAP_START;    //"consumed" heap up to this point
static uintptr_t heap_mapped_end = KHEAP_START;         //mapped pages up to this point
static uintptr_t heap_limit = KHEAP_START + KHEAP_MAX_SIZE; 

typedef struct heap_block{
    uint32_t size;
    uint32_t free;  //1 = free, 0 = used
    struct heap_block *next;
}heap_block_t;


static heap_block_t *free_list = 0;

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


// void *kmalloc(uint32_t size){
//     if(size==0) return 0;

//     //allign at 8 bytes
//     size=align_up(size, 8);

//     if(heap_end + size > heap_limit ){
//         return 0;   //out of heap space
//     }


//     //daca nu avem destule pagini mapate cerem mai multe
//     if(heap_end+size>heap_mapped_end){
//         if(kheap_grow_to(heap_end+size)!=0){
//             return 0;
//         }
//     }

//     void *ptr = (void *)heap_end;
//     heap_end+=size;
//     return ptr;
// }

//ordered by addresses, linked list add element implementation but with blk and addresses 
void insert_block_in_freelist(heap_block_t *blk){
    blk->free = 1;

    heap_block_t *prev = NULL;
    heap_block_t *crt = free_list;

    while( crt != NULL && crt < blk ){
        prev = crt;
        crt = crt -> next;
    }

    blk->next = crt;

    if(prev != NULL ){
        prev -> next = blk;
    }
    else{
        free_list = blk;
    }

    //merge current block with the next one if they end and begin at the same addr

    if( blk-> next != NULL && blk + sizeof(heap_block_t) + blk->size == blk->next ){
        blk->size += blk->next->size + sizeof(heap_block_t);
        blk->next = blk->next->next;
    }

    //merge current block and the previous one if they end and begin at the same addr

    if( prev != NULL && prev + sizeof(heap_block_t) + prev->size == blk){
        prev->size += prev->size + sizeof(heap_block_t);
        prev->next = blk->next;
    }
}

void *kmalloc(uint32_t size){
    if( size == 0 ) return 0;
    
    size = align_up(size, 8);
    heap_block_t *prev = NULL;
    heap_block_t *crt_blk = free_list;

    while(crt_blk != NULL && crt_blk->size < size){
        prev = crt_blk;
        crt_blk = crt_blk->next;
    }

    if(crt_blk == NULL){
        uintptr_t old_end = heap_end;
        if( heap_end + sizeof(heap_block_t) + size > heap_limit ) return 0;

        if(kheap_grow_to(heap_end + sizeof(heap_block_t) + size > heap_limit) != 0) return 0;

        
        uint32_t available = (uint32_t)(heap_mapped_end - old_end);
        if(available <= sizeof(heap_block_t) + size){
            return 0;
        }

        crt_blk = old_end;
        crt_blk->free = 1;
        crt_blk->size = available - sizeof(heap_block_t);
        crt_blk->next = NULL;

        heap_end = old_end + available;

        insert_block_in_freelist(crt_blk);
    }

    prev = NULL;
    crt_blk = free_list;

    while( crt_blk && crt_blk->size < size){
        prev = crt_blk;
        crt_blk = crt_blk->next;
    }
    if(crt_blk == NULL ) return 0;

    if(crt_blk->size >= size + sizeof(heap_block_t) + MIN_SPLIT_SIZE ){
        uintptr_t split_point = crt_blk + sizeof(heap_block_t) + size;
        heap_block_t *new_block = split_point;

        new_block->size = crt_blk->size - size - sizeof(heap_block_t);
        new_block->next = crt_blk->next;
        new_block->free = 1;

        if(prev){
            prev->next = new_block;
        }
        else{
            free_list = new_block;
        }
        crt_blk->size = size;
    }
    else{
        if(prev)
            prev->next = crt_blk->next;
        else
            free_list = crt_blk->next;
    }
    crt_blk->next = NULL;
    crt_blk->free =0;
    return crt_blk + sizeof(heap_block_t);
}


void kfree(void *ptr){
    (void)ptr;
    if(ptr == NULL ) return 0;
    heap_block_t *blk = ptr - sizeof(heap_block_t);

    blk->free = 1;
    blk->next = NULL;
    insert_block_in_freelist(blk);
}
