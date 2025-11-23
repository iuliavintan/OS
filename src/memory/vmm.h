#pragma once
#include "../utils/stdint.h"
#include "../utils/stddef.h"
#include "pmm.h"
#include"../utils/stdint.h"
#include"../utils/util.h"

#define VMM_FLAG_PRESENT 0x001
#define VMM_FLAG_RW      0x002
#define VMM_FLAG_USER    0x004

void vmm_init(void);

int vmm_map_page(uintptr_t virtual_addr, uintptr_t physical_addr, uint32_t flags);
int vmm_unmap_page(uintptr_t virtual_addr);
int vmm_resolve_page(uintptr_t virtual_addr, uintptr_t* physical_addr_out);

