#ifndef _MY_OS_MEMBLOCK_H
#define _MY_OS_MEMBLOCK_H

#include <my-os/types.h>
#include <my-os/list.h>

struct memblock_region {
    phys_addr_t base;
    size_t size;
    struct list_head list;
};

struct memblock_type {
    size_t cnt;
    size_t max;
    size_t total_size;
    struct memblock_region regions;
    char *name;
};

struct memblock {
    struct memblock_type memory;
    struct memblock_type reserved;
};

int memblock_add(phys_addr_t base , size_t size);
int memblock_reserve(phys_addr_t base , size_t size);
int memblock_free(phys_addr_t base, size_t size);
void *memblock_alloc(size_t size, size_t align);
void print_memblock(void);

void memblock_init();
void memblock_mem_mapping();
// phys_addr_t memblock_phys_alloc(size_t size, size_t align);

#endif /* _MY_OS_MEMBLOCK_H */
