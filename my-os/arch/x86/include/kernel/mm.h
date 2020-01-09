#ifndef X86_KERNEL_MM_H
#define X86_KERNEL_MM_H

#include <my-os/types.h>

void early_alloc_pgt_buf(void);
void init_mem_mapping(void);

void *alloc_low_pages(size_t num);

unsigned long init_memory_mapping(unsigned long start, unsigned long end);

void mem_init(void);

#endif /* X86_KERNEL_MM_H */
