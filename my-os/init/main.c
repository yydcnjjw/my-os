#include <asm/sections.h>
#include <my-os/mm_types.h>
#include <asm/page_types.h>
#include <kernel/printk.h>
#include <my-os/memblock.h>


void start_kernel(void) {
    
    struct mm_struct init_mm = {.start_code = (unsigned long)_text,
                                .end_code = (unsigned long)_etext,
                                .start_data = (unsigned long)_data,
                                .end_data = (unsigned long)_edata,
                                .start_brk = (unsigned long)_brk_base
    };

    printk("start code = %p\n", init_mm.start_code);
    printk("end code = %p\n", init_mm.end_code);
    printk("start data = %p\n", init_mm.start_data);
    printk("end data = %p\n", init_mm.end_data);
    printk("start brk = %p\n", init_mm.start_brk);

    /* memblock_reserve(init_mm.start_code, _end_kernel - _text); */
    /* memblock_reserve(0, PAGE_SIZE); */

    /* print_memblock(); */
}
