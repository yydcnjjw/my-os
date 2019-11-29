#include "idt.h"
#include <asm/multiboot2/api.h>
#include <kernel/printk.h>
#include <my-os/types.h>
#include <my-os/start_kernel.h>

extern void x86_64_start_kernel(void *addr, unsigned int magic) {
    init_idt();
    early_serial_init();
    if (-1 == parse_multiboot2(addr, magic)) {
        // TODO: panic
    }
    multiboot2_end_of_ram_pfn();
    start_kernel();
}
