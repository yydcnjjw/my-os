#ifndef _X86_ASM_MULTIBOOT2_API_H
#define _X86_ASM_MULTIBOOT2_API_H

#include <my-os/types.h>
#include <asm/multiboot2/types.h>

void print_memory_map(void);

int parse_multiboot2(void *addr, u64 magic);
void multiboot2_memblock_setup(void);
size_t multiboot2_end_of_ram_pfn(void);
size_t multiboot2_end_of_low_ram_pfn(void);

#endif /* _X86_ASM_MULTIBOOT2_API_H */
