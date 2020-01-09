#ifndef _X86_ASM_PAGE_H
#define _X86_ASM_PAGE_H

#include <asm/page_types.h>

#define __va(x) ((void *)((unsigned long)(x) + PAGE_OFFSET))

static inline unsigned long __phys_addr(unsigned long x) {
    unsigned long y = x - __START_KERNEL_map;
    x = y + ((x > y) ? 0 : (__START_KERNEL_map - PAGE_OFFSET));
    return x;
}

#define __pa(x) __phys_addr((unsigned long)(x))

#define pml4e_index(addr) (((addr) >> PML4E_SHIFT) & (PTRS_PER_PML4E - 1))
#define pdpte_index(addr) (((addr) >> PDPTE_SHIFT) & (PTRS_PER_PDPTE - 1))
#define pde_index(addr) (((addr) >> PDE_SHIFT) & (PTRS_PER_PDE - 1))
#define pte_index(addr) (((addr) >> PTE_SHIFT) & (PTRS_PER_PTE - 1))

#define _KERNPG_TABLE (_PAGE_PRESENT | _PAGE_RW)

#endif /* _X86_ASM_PAGE_H */
