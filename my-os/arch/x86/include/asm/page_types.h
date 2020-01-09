#ifndef _X86_ASM_PAGE_TYPES_H
#define _X86_ASM_PAGE_TYPES_H

#ifndef __ASSEMBLY__
#include <my-os/types.h>

typedef u64 pml4e_t;
typedef u64 pdpte_t;
typedef u64 pde_t;
typedef u64 pte_t;

enum pg_level {
    PG_LEVEL_NONE,
    PG_LEVEL_4K,
    PG_LEVEL_2M,
    PG_LEVEL_1G,
    PG_LEVEL_512G,
    PG_LEVEL_NUM
};

#endif

#define MAX_PHYSMEM_BITS 46
#define MAXMEM (1ULL << MAX_PHYSMEM_BITS)

#define __PAGE_OFFSET_BASE_L4 0xffff888000000000UL
#define __PAGE_OFFSET __PAGE_OFFSET_BASE_L4

#define __START_KERNEL_map 0xffffffff80000000UL

#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)

#define PML5E_SHIFT 48
#define PTRS_PER_PML5E 512

#define PML4E_SHIFT 39
#define PTRS_PER_PML4E 512
#define PML4E_SIZE (1UL << PML4E_SHIFT)
#define PML4E_MASK (~(PML4E_SIZE - 1))

#define PDPTE_SHIFT 30
#define PTRS_PER_PDPTE 512
#define PDPTE_SIZE (1UL << PDPTE_SHIFT)
#define PDPTE_MASK (~(PDPTE_SIZE - 1))

#define PDE_SHIFT 21
#define PTRS_PER_PDE 512
#define PDE_SIZE (1UL << PDE_SHIFT)
#define PDE_MASK (~(PDE_SIZE - 1))

#define PTE_SHIFT 12
#define PTRS_PER_PTE 512
#define PTE_SIZE (1UL << PTE_SHIFT)
#define PTE_MASK (~(PTE_SIZE - 1))

#define _PAGE_BIT_PRESENT 0  /* is present */
#define _PAGE_BIT_RW 1       /* writeable */
#define _PAGE_BIT_USER 2     /* userspace addressable */
#define _PAGE_BIT_PWT 3      /* page write through */
#define _PAGE_BIT_PCD 4      /* page cache disabled */
#define _PAGE_BIT_ACCESSED 5 /* was accessed (raised by CPU) */
#define _PAGE_BIT_PSE 7      /* page size */

#define _PAGE_PRESENT (1UL << _PAGE_BIT_PRESENT)
#define _PAGE_RW (1UL << _PAGE_BIT_RW)
#define _PAGE_USER (1UL << _PAGE_BIT_USER)
#define _PAGE_PWT (1UL << _PAGE_BIT_PWT)
#define _PAGE_PCD (1UL << _PAGE_BIT_PCD)
#define _PAGE_ACCESSED (1UL << _PAGE_BIT_ACCESSED)
#define _PAGE_PSE (1UL << _PAGE_BIT_PSE)

#define _PAGE_KERNEL (_PAGE_RW | _PAGE_PRESENT)

#define __PHYSICAL_MASK_SHIFT 52
#define __PHYSICAL_MASK ((phys_addr_t)((1ULL << __PHYSICAL_MASK_SHIFT) - 1))
#define PHYSICAL_PTE_MASK (((signed long)PTE_MASK) & __PHYSICAL_MASK)
#define PHYSICAL_PDE_MASK (((signed long)PDE_MASK) & __PHYSICAL_MASK)
#define PHYSICAL_PDPTE_MASK (((signed long)PDPTE_MASK) & __PHYSICAL_MASK)

#define EARLY_DYNAMIC_PAGE_TABLES 64
    
#endif /* _X86_ASM_PAGE_TYPES_H */
