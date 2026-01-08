#include"vmm.h"

static uintptr_t kernel_pd_phys;
static uint32_t *kernel_pd; 

// VA (32-bit):
// [31 - 22]  [21 - 12]   [11 - 0]
//  PD idx      PT idx     offset


static inline uint32_t vmm_pd_index(uintptr_t virtual_addr){
    return (virtual_addr >> 22) & 0x3FF;
}

static inline uint32_t vmm_pt_index(uintptr_t virtual_addr){
    return (virtual_addr >> 12) & 0x3FF;
}


int vmm_map_page_pd(uint32_t *pd, uintptr_t virtual_addr, uintptr_t physical_addr, uint32_t flags){
    uint32_t pdi=vmm_pd_index(virtual_addr);
    uint32_t pti=vmm_pt_index(virtual_addr);

    uint32_t pde=pd[pdi];
    uint32_t *pt;

    if(!(pde & VMM_FLAG_PRESENT)){
        uintptr_t pt_phys=pmm_alloc_page();
        if(pt_phys==0) return -1;

        pt=(uint32_t *)pt_phys;
        memset(pt, 0, PAGE_SIZE);

        uint32_t pde_flags = VMM_FLAG_PRESENT | VMM_FLAG_RW;
        if (flags & VMM_FLAG_USER) {
            pde_flags |= VMM_FLAG_USER;
        }
        pd[pdi]=(pt_phys & 0xFFFFF000u) | pde_flags;
    }
    else{
        uintptr_t pt_phys = pde & 0xFFFFF000u;
        pt=(uint32_t *)pt_phys;
        if ((flags & VMM_FLAG_USER) && !(pde & VMM_FLAG_USER)) {
            pd[pdi] = pde | VMM_FLAG_USER;
        }
    }

    uint32_t entry = (physical_addr & 0xFFFFF000u) | (flags & 0xFFF);
    pt[pti] = entry;

    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory"); 

    return 0;
}

int vmm_map_page(uintptr_t virtual_addr, uintptr_t physical_addr, uint32_t flags){
    return vmm_map_page_pd(kernel_pd, virtual_addr, physical_addr, flags);
}

void vmm_init(void){
    kernel_pd_phys=pmm_alloc_page();
    kernel_pd=(uint32_t*)kernel_pd_phys;
    memset(kernel_pd, 0, PAGE_SIZE);


    uintptr_t limit=16*1024*1024;  //16MiB  
    for(uintptr_t pa=0; pa<limit; pa+=PAGE_SIZE){
        vmm_map_page(pa, pa, VMM_FLAG_PRESENT | VMM_FLAG_RW);
    }

    asm volatile("mov %0, %%cr3" :: "r"(kernel_pd_phys) : "memory");

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0|=0x80000000u; //set the paging bit
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

uint32_t *vmm_create_user_pd(uintptr_t *pd_phys_out) {
    uintptr_t pd_phys = pmm_alloc_page();
    if (pd_phys == 0) return NULL;
    uint32_t *pd = (uint32_t *)pd_phys;
    memcpy(pd, kernel_pd, PAGE_SIZE);
    if (pd_phys_out) {
        *pd_phys_out = pd_phys;
    }
    return pd;
}

uintptr_t vmm_kernel_pd_phys(void) {
    return kernel_pd_phys;
}

void vmm_switch_pd(uintptr_t pd_phys) {
    asm volatile("mov %0, %%cr3" :: "r"(pd_phys) : "memory");
}

int vmm_unmap_page(uintptr_t virtual_addr){
    uint32_t pdi=vmm_pd_index(virtual_addr);
    uint32_t pti=vmm_pt_index(virtual_addr);

    uint32_t pde=kernel_pd[pdi];
    if(!(pde & VMM_FLAG_PRESENT)){
        return -1;                  //there s no page table
    }

    uint32_t *pt= (uint32_t *)(pde & 0xFFFFF000u);
    if(!(pt[pti] & VMM_FLAG_PRESENT)){
        return -1;                  //the page isnt mapped
    }

    pt[pti]=0; //unmap the page

    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory"); 

    return 0;
}

int vmm_resolve_page(uintptr_t virtual_addr, uintptr_t* physical_addr_out){
    if(physical_addr_out == 0) return -1;

    uint32_t pdi = vmm_pd_index(virtual_addr);
    uint32_t pti = vmm_pt_index(virtual_addr);

    uint32_t pde = kernel_pd[pdi];
    if(!(pde & VMM_FLAG_PRESENT)){
        return -1;
    }
    
    uint32_t *pt = (uint32_t *)(pde & 0xFFFFF000u);
    uint32_t pte = pt[pti];
    if(!(pte & VMM_FLAG_PRESENT)){
        return -1;
    }

    uintptr_t base = (uintptr_t)(pte & 0xFFFFF000u);
    base|=virtual_addr & 0xFFFu; //add offset
    *physical_addr_out=base;
    return 0;
}
