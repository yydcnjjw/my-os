#ifndef _MY_OS_MEMBLOCK_H
#define _MY_OS_MEMBLOCK_H

#include <my-os/types.h>

struct memblock_region {
    phys_addr_t base;
    size_t size;
};

struct memblock_type {
    size_t cnt;
    size_t max;
    size_t total_size;
    struct memblock_region *regions;
    char *name;
};

struct memblock {
    bool bottom_up; /* is bottom up direction? */
    size_t current_limit;
    struct memblock_type memory;
    struct memblock_type reserved;
};

void *memblock_alloc(size_t size, phys_addr_t align);
phys_addr_t memblock_phys_alloc(size_t size, phys_addr_t align);

#endif /* _MY_OS_MEMBLOCK_H */
