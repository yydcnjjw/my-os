#include <asm/multiboot2/api.h>
#include <asm/page_types.h>
#include <asm/sections.h>
#include <asm/apic.h>

#include <my-os/memblock.h>
#include <my-os/mm_types.h>

#include <kernel/printk.h>

void start_kernel(void) {

    struct mm_struct init_mm = {.start_code = (unsigned long)_text,
                                .end_code = (unsigned long)_etext,
                                .start_data = (unsigned long)_data,
                                .end_data = (unsigned long)_edata,
                                .start_brk = (unsigned long)_brk_base};

    printk("start code = %p\n", init_mm.start_code);
    printk("end code = %p\n", init_mm.end_code);
    printk("start data = %p\n", init_mm.start_data);
    printk("end data = %p\n", init_mm.end_data);
    printk("start brk = %p\n", init_mm.start_brk);
    memblock_init();
    memblock_reserve((phys_addr_t)_boot_start, _boot_end - _boot_start);
    memblock_reserve(0, PAGE_SIZE);

    memblock_reserve((phys_addr_t)_start - 0xffffffff80000000, _end - _start);

    multiboot2_end_of_ram_pfn();
    multiboot2_memblock_setup();
    print_memblock();
    local_apic_init();
}
