#include <my-os/memblock.h>

#include <my-os/kernel.h>
#include <my-os/limits.h>
#include <my-os/stack_alloc.h>
#include <kernel/printk.h>

#define INIT_MEMBLOCK_REGIONS 128

STACK_POOL(memblock_region_pool, INIT_MEMBLOCK_REGIONS, struct memblock_region);

static struct memblock __init_memblock = {
    .memory.regions =
        {
            .list = LIST_HEAD_INIT(__init_memblock.memory.regions.list),
        },
    .memory.cnt = 0,
    .memory.max = INIT_MEMBLOCK_REGIONS,
    .memory.name = "memory",
    .reserved.regions =
        {
            .list = LIST_HEAD_INIT(__init_memblock.reserved.regions.list),
        },
    .reserved.cnt = 0,
    .reserved.max = INIT_MEMBLOCK_REGIONS,
    .reserved.name = "reserved"};

static inline phys_addr_t memblock_cap_size(phys_addr_t base, size_t *size) {
    return *size = min(*size, PHYS_ADDR_MAX - base);
}

static inline struct memblock_region *new_memblock_region(phys_addr_t base,
                                                          size_t size) {
    struct memblock_region *rgx = stack_alloc(&memblock_region_pool);
    rgx->base = base;
    rgx->size = size;
    return rgx;
}

// add memblock_region to tail of rgn
static void add_memblock_region_tail(struct memblock_type *type,
                                     struct memblock_region *rgn,
                                     struct memblock_region *new) {
    list_add(&new->list, &rgn->list);
    type->cnt++;
    type->total_size += rgn->size;
}

static void add_memblock_region_front(struct memblock_type *type,
                                      struct memblock_region *rgn,
                                      struct memblock_region *new) {
    add_memblock_region_tail(type, list_prev_entry(rgn, list), new);
}

static void memblock_merge_regions(struct memblock_type *type) {
    struct memblock_region *region;
    struct list_head *head = &type->regions.list;
    list_for_each_entry(region, head, list) {
        struct memblock_region *next = list_next_entry(region, list);
        if (&next->list == head) {
            break;
        }
        if (region->base + region->size != next->base) {
            continue;
        }

        region->size += next->size;
        list_del(&next->list);
        type->cnt--;
    }
}

static int memblock_add_range(struct memblock_type *type, phys_addr_t base,
                              size_t size) {

    phys_addr_t end = base + memblock_cap_size(base, &size);

    if (!size)
        return 0;

    if (type->cnt == 0) {
        struct memblock_region *region = new_memblock_region(base, size);
        list_add(&region->list, &type->regions.list);
        type->cnt++;
        type->total_size = size;
        return 0;
    }

    // TODO: resize memblock region pool
    bool insert = true;

    struct memblock_region *region;
    list_for_each_entry(region, &type->regions.list, list) {
        phys_addr_t rbase = region->base;
        phys_addr_t rend = rbase + region->size;

        if (rbase >= end)
            break;

        if (rend <= base)
            continue;

        if (rbase > base) {
            if (insert) {
                struct memblock_region *rgn =
                    new_memblock_region(base, rbase - base);
                add_memblock_region_front(type, region, rgn);
            }
        }

        base = min(rend, end);
    }

    if (base < end) {
        if (insert) {
            struct memblock_region *rgn = new_memblock_region(base, end - base);
            add_memblock_region_front(type, region, rgn);
        }
    }

    if (!insert) {
        // resize memblock region pool
        return 0;
    } else {
        memblock_merge_regions(type);
        return 0;
    }
}

static int memblock_remove_range(struct memblock_type *type, phys_addr_t base,
                                 size_t size) {

    struct memblock_region *region;
    struct list_head *head = &type->regions.list;

    phys_addr_t end = base + memblock_cap_size(base, &size);

    if (!size)
        return 0;

    list_for_each_entry(region, head, list) {
        phys_addr_t rbase = region->base;
        phys_addr_t rend = rbase + region->size;

        if (rbase >= end)
            break;

        if (rend <= base)
            continue;

        if (rbase < base) {
            region->base = base;
            region->size -= base - rbase;
            struct memblock_region *rgx =
                new_memblock_region(rbase, base - rbase);
            add_memblock_region_front(type, region, rgx);
        }

        if (rend > end) {
            region->size -= rend - end;
            struct memblock_region *rgx = new_memblock_region(end, rend - end);
            add_memblock_region_tail(type, region, rgx);
        }

        if (region->size == size && region->base == base) {
            list_del(&region->list);
            break;
        }
    }

    return 0;
}

static int memblock_remove(phys_addr_t base, size_t size) {
    phys_addr_t end = base + size - 1;
    printk("memblock remove: [%p-%p]\n", base, end);    
    return memblock_remove_range(&__init_memblock.memory, base, size);
}

int memblock_free(phys_addr_t base, size_t size) {
    phys_addr_t end = base + size - 1;
    printk("memblock free: [%p-%p]\n", base, end);
    return memblock_remove_range(&__init_memblock.reserved, base, size);
}

int memblock_reserve(phys_addr_t base, size_t size) {
    phys_addr_t end = base + size - 1;
    printk("memblock reserve: [%p-%p]\n", base, end);
    return memblock_add_range(&__init_memblock.reserved, base, size);
}

int memblock_add(phys_addr_t base, size_t size) {
    phys_addr_t end = base + size - 1;
    printk("memblock add: [%p-%p]\n", base, end);
    return memblock_add_range(&__init_memblock.memory, base, size);    
}

void print_memblock_type(struct memblock_type *type) {
    printk("name: %s\n", type->name);
    printk("total size = %#x, region size = %d\n", type->total_size, type->cnt);
    
    struct memblock_region *region;
    list_for_each_entry(region, &type->regions.list, list) {
        phys_addr_t end = region->base + region->size - 1;
        printk("memblock reserve: [%p-%p]\n", region->base, end);
    }
}

void print_memblock(void) {
    printk("memblock: \n");
    print_memblock_type(&__init_memblock.memory);
    print_memblock_type(&__init_memblock.reserved);
}

#define __round_mask(x, y) ((typeof(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))


static phys_addr_t memblock_find_in_range(size_t size, size_t align) {
    struct list_head *head =   &__init_memblock.memory.regions.list;
    struct memblock_region *region;

    phys_addr_t addr;
    list_for_each_entry_reverse(region, head, list) {
        phys_addr_t rstart = region->base;
        phys_addr_t rend = rstart + region->size;

        addr = round_down(rend - size, align);
        if (addr < rend && rend - addr >= size) {
            return addr;
        }
    }
    return 0;
}

void *memblock_alloc(size_t size, size_t align) {
    phys_addr_t addr = memblock_find_in_range(size, align);
    if (addr) {
        memblock_reserve(addr, size);
        return (void *)addr;
    }

    return 0;
}



void memblock_init() {
    init_stack_pool(&memblock_region_pool);
}
