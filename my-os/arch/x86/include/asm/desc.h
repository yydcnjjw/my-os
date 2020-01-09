#ifndef _X86_ASM_DESC_H
#define _X86_ASM_DESC_H

#include <asm/desc_defs.h>
#include <asm/page_types.h>

#define GDT_ENTRIES 7

#ifndef __ASSEMBLY__

struct gdt_page {
    struct desc_struct gdt[GDT_ENTRIES];
} __attribute__((aligned(PTE_SIZE)));

extern struct gdt_page gdt_page;

extern void early_init_idt(void);
#endif

#endif /* _X86_ASM_DESC_H */
