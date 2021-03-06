#include <asm/acpi.h>
#include <asm/multiboot2/api.h>
#include <asm/page_types.h>
#include <kernel/printk.h>
#include <my-os/memblock.h>
#include <my-os/string.h>

static struct multiboot_tag_mmap *mmap_tag;

#define for_each_mmap_entries(entry)                                           \
    for (mmap = mmap_tag->entries;                                             \
         (void *)mmap < (void *)mmap_tag + mmap_tag->size;                     \
         mmap = (void *)mmap + mmap_tag->entry_size)

#define MAX_ARCH_PFN MAXMEM >> PTE_SHIFT

static size_t multiboot2_end_pfn(size_t limit_pfn,
                                 enum multiboot_memroy_type type) {
    size_t last_pfn = 0;
    multiboot_memory_map_t *mmap;
    for_each_mmap_entries(mmap) {
        if (mmap->type != type) {
            continue;
        }

        unsigned long start_pfn = mmap->addr >> PTE_SHIFT;
        unsigned long end_pfn = (mmap->addr + mmap->len) >> PTE_SHIFT;

        if (start_pfn >= limit_pfn)
            continue;

        if (end_pfn > limit_pfn) {
            last_pfn = limit_pfn;
            break;
        }

        if (end_pfn > last_pfn) {
            last_pfn = end_pfn;
        }
    }

    if (last_pfn > MAX_ARCH_PFN) {
        last_pfn = MAX_ARCH_PFN;
    }

    printk("last_pfn = %#x, max_arch_pfn=%#x\n", last_pfn, MAX_ARCH_PFN);
    return last_pfn;
}

size_t multiboot2_end_of_ram_pfn(void) {
    return multiboot2_end_pfn(MAX_ARCH_PFN, MULTIBOOT_MEMORY_AVAILABLE);
}

size_t multiboot2_end_of_low_ram_pfn(void) {
    return multiboot2_end_pfn(1UL << (32 - PTE_SHIFT),
                              MULTIBOOT_MEMORY_AVAILABLE);
}

void print_memory_map(void) {
    printk("mmap:\n");
    multiboot_memory_map_t *mmap;
    for_each_mmap_entries(mmap) {
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
        case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
            struct multiboot_tag_old_acpi *old_acpi_tag =
                (struct multiboot_tag_old_acpi *)tag;
            struct RSDPDescriptor rsdp;
            memcpy(&rsdp, old_acpi_tag->rsdp, sizeof(struct RSDPDescriptor));
            rsdt = (void *)(phys_addr_t)rsdp.rsdt_address;
            break;
        }
        case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
            struct multiboot_tag_new_acpi *new_acpi_tag =
                (struct multiboot_tag_new_acpi *)tag;
            printk("new acpi tag size %d\n", new_acpi_tag->size);
            break;
        }
        }

        tag = (void *)tag + align(tag->size, 8);
    }
    return 0;
}

void multiboot2_memblock_setup(void) {
    multiboot_memory_map_t *mmap;
    for_each_mmap_entries(mmap) {
        if (MULTIBOOT_MEMORY_AVAILABLE == mmap->type) {
            memblock_add(mmap->addr, mmap->len);
        } else if (MULTIBOOT_MEMORY_RESERVED == mmap->type) {
            memblock_reserve(mmap->addr, mmap->len);
        }
    }
}
