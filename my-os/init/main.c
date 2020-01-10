#include <asm/apic.h>
#include <asm/multiboot2/api.h>
#include <asm/page_types.h>
#include <asm/sections.h>

#include <my-os/memblock.h>
#include <my-os/mm_types.h>

#include <kernel/mm.h>
#include <kernel/printk.h>
#include <my-os/buddy_alloc.h>

void start_kernel(void) {

    printk("lma end %p\n", (unsigned long)KERNEL_LMA_END);

    init_mm.start_code = (unsigned long)_text;
    init_mm.end_code = (unsigned long)_etext;
    init_mm.start_data = (unsigned long)_data;
    init_mm.end_data = (unsigned long)_edata;
    init_mm.start_brk = (unsigned long)_brk_base;

    printk("start code = %p\n", init_mm.start_code);
    printk("end code = %p\n", init_mm.end_code);
    printk("start data = %p\n", init_mm.start_data);
    printk("end data = %p\n", init_mm.end_data);
    printk("start brk = %p\n", init_mm.start_brk);

    memblock_init();
    early_alloc_pgt_buf();

    size_t end_pfn = multiboot2_end_of_ram_pfn();
    multiboot2_memblock_setup();
    print_memblock();

    init_mem_mapping();

    /* mem_init(); */

    struct buddy_alloc *buddy =
        buddy_new((phys_addr_t)KERNEL_LMA_END, end_pfn << PTE_SHIFT);

    void *addr[10];
    for (int i = 0; i < 10; i++) {
        void *p = buddy_alloc(buddy, 11);
        if (!p) {
            printk("no memory\n");
            break;
        }
        addr[i] = p;
        buddy_size(buddy, addr[i]);
        printk("\n");
    }
    for (int i = 0; i < 2; i++) {
        buddy_free(buddy, addr[i]);
    }
    for (int i = 0; i < 2; i++) {
        addr[i] = buddy_alloc(buddy, 10);
        buddy_size(buddy, addr[i]);
        printk("\n");
    }

    local_apic_init();
}
