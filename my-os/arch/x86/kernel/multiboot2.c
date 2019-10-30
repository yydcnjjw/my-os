#include <asm/page_types.h>
#include <asm/multiboot2/api.h>
#include <kernel/printk.h>

static struct multiboot_tag_mmap *mmap_tag;

#define foreach_mmap_entries                                                   \
    for (multiboot_memory_map_t *mmap = mmap_tag->entries;                     \
         (void *)mmap < (void *)mmap_tag + mmap_tag->size;                     \
         mmap = (void *)mmap + mmap_tag->entry_size)

static size_t multiboot2_end_pfn(u32 type) {
    foreach_mmap_entries {
        if (mmap->type != type) {
            continue;
        }

        unsigned long start_pfn = mmap->addr >> PAGE_SHIFT;
        unsigned long end_pfn = (mmap->addr + mmap->len) >> PAGE_SHIFT;
        
    }
}

void print_memory_map() {
    printk("mmap:\n");
    foreach_mmap_entries {
        printk("base_addr = %p, length = %p, ", mmap->addr, mmap->len);
        switch (mmap->type) {
        case MULTIBOOT_MEMORY_AVAILABLE:
            printk("type = available");
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            printk("type = reserved");
            break;
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            printk("type = acpi reclaimable");
            break;
        case MULTIBOOT_MEMORY_NVS:
            printk("type = nvs");
            break;
        case MULTIBOOT_MEMORY_BADRAM:
            printk("type = badram");
            break;
        }
        printk("\n");        
    }
}

#define align(n, align) ((n + align - 1) & ~(align - 1))

int parse_multiboot2(void *addr, u64 magic) {
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printk("Invalid magic number: %#x\n", (unsigned)magic);
        return -1;
    }

    if ((u64)addr & 0x7) {
        printk("Unaligned mbi: %#x\n", addr);
        return -1;
    }

    u32 size = *(u32 *)addr;
    printk("Announced mbi size %#x\n", size);

    struct multiboot_tag *tag = addr + 8;
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {

        // printk("Tag %#x, size %#x\n", tag->type, tag->size);
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
            struct multiboot_tag_basic_meminfo *meminfo_tag =
                (struct multiboot_tag_basic_meminfo *)tag;
            printk("mem_lower = %#xKB, mem_upper = %#xKB\n",
                   meminfo_tag->mem_lower, meminfo_tag->mem_upper);
            printk("mem_lower = %uKB, mem_upper = %uKB\n",
                   meminfo_tag->mem_lower, meminfo_tag->mem_upper);
            break;
        }
        case MULTIBOOT_TAG_TYPE_MMAP: {
            mmap_tag = (struct multiboot_tag_mmap *)tag;
            print_memory_map();
            break;
        }
        }

        tag = (void *)tag + align(tag->size, 8);
    }
    return 0;
}
