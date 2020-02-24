#include <asm/desc.h>
#include <asm/io.h>
#include <asm/multiboot2/api.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>

#include <kernel/printk.h>
#include <my-os/mm_types.h>
#include <kernel/mm.h>

#include <my-os/start_kernel.h>
#include <my-os/string.h>

char early_stack[4096] = {0};

static inline void irq_disable(void) { asm volatile("cli" : : : "memory"); }

static inline void irq_enable(void) { asm volatile("sti" : : : "memory"); }

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define ICW1_ICW4 0x01      /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

void pic_remap(int offset1, int offset2) {
    u8 a1, a2;

    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(ICW1_INIT | ICW1_ICW4, PIC1_COMMAND);
    outb(ICW1_INIT | ICW1_ICW4, PIC2_COMMAND);
    outb(offset1, PIC1_DATA);
    outb(offset2, PIC2_DATA);
    outb(4, PIC1_DATA);
    outb(2, PIC2_DATA);
    outb(ICW4_8086, PIC1_DATA);
    outb(ICW4_8086, PIC2_DATA);

    // set mask
    outb(a1, PIC1_DATA);
    outb(a2, PIC2_DATA);
}

void pic_disable() {
    outb(0xff, PIC1_DATA);
    outb(0xff, PIC2_DATA);
}

extern pml4e_t early_pml4t[PTRS_PER_PML4E];
extern pde_t early_dynamic_page_tables[EARLY_DYNAMIC_PAGE_TABLES][PTRS_PER_PTE];
static size_t next_early_pgt;
unsigned long __force_order;
static void reset_early_page_table(void) {
    memset(early_pml4t, 0, sizeof(pml4e_t) * (PTRS_PER_PML4E - 1));
    next_early_pgt = 0;
    write_cr3((phys_addr_t)early_pml4t);
}

int early_make_pgtable(unsigned long address) {
    phys_addr_t physaddr = address - __PAGE_OFFSET;
    pde_t pde = (physaddr & PDE_MASK) + (_PAGE_PSE | _PAGE_PRESENT | _PAGE_RW);

    // TODO: early_pml4t to __START_KERNEL_map address
    if (physaddr >= MAXMEM || (phys_addr_t)early_pml4t != read_cr3_pa()) {
        return -1;
    }
    pml4e_t *pml4e_p;
    pml4e_t pml4e;
again:

    pml4e_p = &early_pml4t[pml4e_index(address)];
    pml4e = *pml4e_p;

    pdpte_t *pdpte_p;
    if (pml4e) {
        pdpte_p = (pdpte_t *)((pml4e & PTE_PFN_MASK) + __START_KERNEL_map);
    } else {
        if (next_early_pgt >= EARLY_DYNAMIC_PAGE_TABLES) {
            reset_early_page_table();
            goto again;
        }
        pdpte_p = early_dynamic_page_tables[next_early_pgt++];
        bzero(pdpte_p, sizeof(pdpte_t) * PTRS_PER_PDPTE);
        *pml4e_p =
            (pml4e_t)pdpte_p - __START_KERNEL_map + (_PAGE_RW | _PAGE_PRESENT);
    }

    pdpte_p += pdpte_index(address);
    pdpte_t pdpte = *pdpte_p;

    pde_t *pde_p;
    if (pdpte) {
        pde_p = (pde_t *)((pdpte & PTE_PFN_MASK) + __START_KERNEL_map);
    } else {
        if (next_early_pgt >= EARLY_DYNAMIC_PAGE_TABLES) {
            reset_early_page_table();
            goto again;
        }
        
        pde_p = early_dynamic_page_tables[next_early_pgt++];
        bzero(pde_p, sizeof(pde_t) * PTRS_PER_PDE);
        *pdpte_p =
            (pdpte_t)pde_p - __START_KERNEL_map + (_PAGE_RW | _PAGE_PRESENT);
    }
    pde_p[pde_index(address)] = pde;
    return 0;
}

void x86_64_start_kernel(void *addr, unsigned int magic) {
    early_vga_init();
    early_serial_init();
    
    early_init_idt();
    pic_remap(0x20, 0x28);
    pic_disable();
    irq_enable();

    pml4e_t *pml4e_page = alloc_low_pages(1);
    pml4e_page[511] = early_pml4t[511];
    init_mm.top_page = pml4e_page;
    
    if (-1 == parse_multiboot2(addr, magic)) {
        // TODO: panic
    }
    start_kernel();

    while (true)
        ;
}
