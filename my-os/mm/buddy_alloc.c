#include <asm/multiboot2/api.h>
#include <asm/page.h>
#include <asm/sections.h>
#include <my-os/buddy_alloc.h>
#include <my-os/kernel.h>
#include <my-os/log2.h>
#include <my-os/mm_types.h>
#include <my-os/types.h>

#include <kernel/mm.h>
#include <kernel/printk.h>

struct buddy_free_area {
    u8 max_order;
    u8 area[];
};

struct buddy_alloc {
    size_t start_pfn;
    size_t free_area_num;
    struct buddy_free_area **free_area;
};

static struct buddy_alloc *buddy_base;

#define is_align(n, align) (!((n) & ((align)-1)))

#define MAX_ORDER 11
#define FREE_AREA_PFN (1 << MAX_ORDER)
#define FREE_AREA_NODE_NUM ((FREE_AREA_PFN << 1) - 1)

/* #define buddy_node(buddy, i, j)                                                \ */
/*     ((buddy)->free_area + ((i)*FREE_AREA_NODE_NUM + (j))) */

long buddy_alloc_pfn(struct buddy_alloc *buddy, size_t order);

struct buddy_alloc *buddy_new(phys_addr_t start, phys_addr_t end) {
    printk("buddy: %#x-%#x\n", start, end);
    if (end < start) {
        return NULL;
    }
    size_t mem_size = end - start;
    printk("mem size: %#x\n", mem_size);

    if (!is_align(mem_size, PAGE_SIZE)) {
        return NULL;
    }
    size_t pfn_num = mem_size >> PAGE_SHIFT;

    size_t free_area_order = MAX_ORDER;
    size_t free_area_num = 0;
    struct buddy_free_area **brk_free_area_base = NULL;
    u8 *free_area_base = __va(start);
    while (pfn_num) {
        size_t map_page = 1 << free_area_order;
        if (pfn_num < map_page) {
            // TODO: do not use buddy block if order < MAX_ORDER
            // consider use brk
            // because the use of 4K memory block can waste memory
            free_area_order--;
            continue;
        }
        size_t node_num = (map_page << 1) - 1;
        size_t node_order = free_area_order + 2;

        struct buddy_free_area **area =
            extend_brk(sizeof(struct buddy_free_area *), 8);
        *area = (struct buddy_free_area *)free_area_base;

        free_area_base[0] = free_area_order;
        free_area_base += 1;
        for (size_t j = 0; j < node_num; j++) {
            if (is_power_of_2(j + 1)) {
                node_order--;
            }
            free_area_base[j] = node_order;
        }
        free_area_num++;
        if (!brk_free_area_base)
            brk_free_area_base = area;

        free_area_base += node_num;
        pfn_num -= map_page;
    }

    struct buddy_alloc *buddy = extend_brk(sizeof(struct buddy_alloc), 8);
    buddy->start_pfn = start >> PAGE_SHIFT;
    buddy->free_area_num = free_area_num;
    buddy->free_area = brk_free_area_base;

    size_t reserve_mem = __pa(free_area_base) - start;
    printk("reserve memory %#x\n", reserve_mem);

    // mark reserve
    size_t reserve_num = reserve_mem >> PAGE_SHIFT;
    while (reserve_num) {
        size_t i = ilog2(reserve_num);
        buddy_alloc_pfn(buddy, i);
        reserve_num -= 1 << i;
    }
    return buddy;
}

#define LEFT_LEAF(index) ((index)*2 + 1)
#define RIGHT_LEAF(index) ((index)*2 + 2)
#define PARENT(index) (((index) + 1) / 2 - 1)

long _buddy_alloc(struct buddy_free_area *area, size_t order) {
    if (!area) {
        return -1;
    }

    size_t index = 0;
    if (area->area[index] < order + 1) {
        return -1;
    }

    size_t node_order;
    for (node_order = area->max_order + 1; node_order != order + 1;
         node_order--) {
        if (area->area[LEFT_LEAF(index)] >= order + 1) {
            index = LEFT_LEAF(index);
        } else {
            index = RIGHT_LEAF(index);
        }
    }
    area->area[index] = 0;
    size_t ret = index;
    while (index) {
        index = PARENT(index);
        area->area[index] =
            max(area->area[LEFT_LEAF(index)], area->area[RIGHT_LEAF(index)]);
    }

    return ret;
}

long buddy_alloc_pfn(struct buddy_alloc *buddy, size_t order) {
    if (!buddy || order > MAX_ORDER)
        return -1;

    size_t i;
    size_t map_pfn = 0;
    for (i = 0; i < buddy->free_area_num; i++) {
        if (buddy->free_area[i]->area[0] >= order + 1) {
            break;
        }
        map_pfn += 1 << buddy->free_area[i]->max_order;
    }

    if (i == buddy->free_area_num) {
        return -1;
    }

    long index = _buddy_alloc(buddy->free_area[i], order);
    if (index == -1) {
        return -1;
    }

    map_pfn +=
        (index + 1) * (1 << order) - (1 << buddy->free_area[i]->max_order);
    /* printk("alloc order %d, free area %d, index %d, pfn %#x\n", order, i, index, */
    /*        map_pfn); */
    return buddy->start_pfn + map_pfn;
}

int buddy_get_free_area_index(struct buddy_alloc *buddy, long pfn,
                              size_t *map_page_offset) {
    size_t pfn_offset = pfn - buddy->start_pfn;
    size_t index = 0;
    size_t map_page = 1 << buddy->free_area[index]->max_order;
    while (pfn_offset >= map_page) {
        if (index > buddy->free_area_num) {
            return -1;
        }
        map_page += 1 << buddy->free_area[index]->max_order;
        index++;
    }
    *map_page_offset = map_page - (1 << buddy->free_area[index]->max_order);
    return index;
}

int _buddy_free(struct buddy_free_area *area, size_t area_offset) {
    size_t index = area_offset + (1 << area->max_order) - 1;
    size_t node_order = 1;
    for (; area->area[index]; index = PARENT(index)) {
        node_order++;
        if (index == 0) {
            break;
        }
    }

    area->area[index] = node_order;

    while (index) {
        index = PARENT(index);
        node_order++;
        size_t left_order = area->area[LEFT_LEAF(index)];
        size_t right_order = area->area[RIGHT_LEAF(index)];
        if (left_order + right_order == node_order) {
            area->area[index] = node_order;
        } else {
            area->area[index] = max(left_order, right_order);
        }
    }
    return 0;
}

int buddy_free(struct buddy_alloc *buddy, long pfn) {
    size_t map_page_offset = 0;
    int free_area_index =
        buddy_get_free_area_index(buddy, pfn, &map_page_offset);

    if (free_area_index == -1) {
        return -1;
    }
    size_t page_offset = pfn - buddy->start_pfn;
    size_t area_offset = page_offset - map_page_offset;

    return _buddy_free(buddy->free_area[free_area_index], area_offset);
}

size_t buddy_size(struct buddy_alloc *buddy, long pfn) {
    size_t map_page_offset = 0;
    int free_area_index =
        buddy_get_free_area_index(buddy, pfn, &map_page_offset);

    if (free_area_index == -1) {
        return -1;
    }
    size_t page_offset = pfn - buddy->start_pfn;
    size_t area_offset = page_offset - map_page_offset;

    struct buddy_free_area *area = buddy->free_area[free_area_index];
    size_t index = area_offset + (1 << area->max_order) - 1;
    size_t node_order = 1;
    for (; area->area[index]; index = PARENT(index))
        node_order++;

    printk("size order %d, free area %d, index %d, pfn %#x\n", --node_order,
           free_area_index, index, pfn);
    return 1 << --node_order << PAGE_SHIFT;
}

void init_buddy_alloc() {
    buddy_base = buddy_new((phys_addr_t)KERNEL_LMA_END, end_pfn << PAGE_SHIFT);
}

phys_addr_t _alloc_pages(size_t order) {
    long pfn = buddy_alloc_pfn(buddy_base, order);
    if (pfn == -1) {
        return 0;
    }
    return pfn << PAGE_SHIFT;
}

struct page *alloc_pages(size_t order) {
    long pfn = buddy_alloc_pfn(buddy_base, order);
    if (pfn == -1) {
        return NULL;
    }
    return pfn_to_page(pfn);
}

void free_pages(struct page *page) {
    buddy_free(buddy_base, page_to_pfn(page));
}
size_t pages_size(struct page *page) {
    return buddy_size(buddy_base, page_to_pfn(page));
}
