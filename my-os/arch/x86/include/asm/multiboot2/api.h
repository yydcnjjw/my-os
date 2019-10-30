#ifndef _X86_ASM_MULTIBOOT2_API_H
#define _X86_ASM_MULTIBOOT2_API_H

#include <my-os/types.h>
#include <asm/multiboot2/types.h>

void print_memory_map();

int parse_multiboot2(void *addr, u64 magic);

#endif /* _X86_ASM_MULTIBOOT2_API_H */
