#include "idt.h"
#include "kernel/printk.h"
#include "multiboot2.h"

int parse_multiboot2(void *addr, unsigned long long magic) {
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printk("Invalid magic number: %#x\n", (unsigned)magic);
        return -1;
    }

    if ((unsigned long long)addr & 0x7) {
        printk("Unaligned mbi: %#x\n", addr);
        return -1;
    }

    unsigned int size = *(unsigned int *)addr;
    printk("Announced mbi size %#x\n", size);

    struct multiboot_tag *tag;
    for (tag = (struct multiboot_tag *)(addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                        ((tag->size + 0x7) & ~0x7))) {

        // printk("Tag %#x, size %#x\n", tag->type, tag->size);
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
            printk("mem_lower = %#xKB, mem_upper = %#xKB\n",
                   ((struct multiboot_tag_basic_meminfo *)tag)->mem_lower,
                   ((struct multiboot_tag_basic_meminfo *)tag)->mem_upper);
            printk("mem_lower = %uKB, mem_upper = %uKB\n",
                   ((struct multiboot_tag_basic_meminfo *)tag)->mem_lower,
                   ((struct multiboot_tag_basic_meminfo *)tag)->mem_upper);            
            break;
        }
    }
    return 0;
}

extern void x86_64_start_kernel(void *addr, unsigned int magic) {
    init_idt();
    early_serial_init();
    parse_multiboot2(addr, magic);
}
