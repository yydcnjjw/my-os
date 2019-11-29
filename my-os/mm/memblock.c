->#include <my-os/memblock.h>

#include <my-os/kernel.h>
#include <my-os/limits.h>
#include <my-os/stack_alloc.h>

#define INIT_MEMBLOCK_REGIONS 32

STACK_POOL(memblock_region_pool,
           sizeof(struct memblock_region) * INIT_MEMBLOCK_REGIONS);

struct memblock __init_memblock = {
    .memory.regions = LIST_HEAD_INIT(__init_memblock.memory.regions.list),
    .memory.cnt = 1,
    .memory.max = INIT_MEMBLOCK_REGIONS,
    .memory.name = "memory",
    .reserved.regions = LIST_HEAD_INIT(__init_memblock.reserved.regions.list),
    .reserved.cnt = 1,
    .reserved.max = INIT_MEMBLOCK_REGIONS,
    .reserved.name = "reserved"};

static inline phys_addr_t memblock_cap_size(phys_addr_t base,
                                            phys_addr_t *size) {
    return *size = min(*size, PHYS_ADDR_MAX - base);
}


int memblock_add_range(struct memblock_type *type, phys_addr_t base,
                       phys_addr_t size) {

    struct memblock_region *region;
    list_for_each_entry(region, &type->regions, list) {
        
    }

    /* phys_addr_t end = base + memblock_cap_size(base, &size); */

    /* int idx; */
    /* if (type->regions[0].size == 0) { */
    /*     type->regions[0].base = base; */
    /*     type->regions[0].size = size; */
    /*     type->total_size = size; */
    /*     return 0; */
    /* } */

    /* struct memblock_region *rgn; */
    /* for_eac (idx, type, rgn) { */
    /*     phys_addr_t rbase = rgn->base; */
    /*     phys_addr_t rend = rbase + rgn->size; */
    /*     if (rbase >= end) */
    /*         break; */
    /*     if (rend <= base) */
    /*         continue; */

    /*     if (rbase > base) { */
    /*     } */
    /* } */
}
