#pragma once

#include "../stdint.h"


void kheap_init(void);

void *kmalloc(uint32_t size);

void kfree( void *ptr );


void kheap_run_basic_tests(void);
void kheap_test_with_logging(void);

void heap_dump(void);
void kheap_run_basic_tests(void);
