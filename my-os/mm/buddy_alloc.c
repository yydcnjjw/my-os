#include <asm/page.h>
#include <my-os/buddy_alloc.h>
#include <my-os/kernel.h>
#include <my-os/types.h>

#include <kernel/printk.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

struct buddy_free_area {
    u8 max_order;
    u8 *free_area;
};

struct buddy_alloc {
    void *base_addr;
    /* unsigned long size; */
    size_t free_area_num;
    u8 *free_area;
};

#define is_align(n, align) (!((n) & ((align)-1)))
#define is_power_of_2(x) (!((x) & ((x)-1)))

#define MAX_ORDER 11
#define FREE_AREA_PFN (1 << MAX_ORDER)
#define FREE_AREA_NODE_NUM ((FREE_AREA_PFN << 1) - 1)

#define buddy_node(buddy, i, j)                                                \
    ((buddy)->free_area + ((i)*FREE_AREA_NODE_NUM + (j)))

struct buddy_alloc global_buddy;

int log2(int x) {
    float fx;
    unsigned long ix, exp;

    fx = (float)x;
    ix = *(unsigned long *)&fx;
    exp = (ix >> 23) & 0xFF;

    return exp - 127;
}

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

    size_t page_size = round_down((mem_size >> PAGE_SHIFT), 2);
    if ((mem_size >> PAGE_SHIFT) != page_size) {
        printk("page rest\n");
    }

    size_t free_area_num = page_size / FREE_AREA_PFN;
    printk("free area num %d\n", free_area_num);
    size_t rest_page = page_size % FREE_AREA_PFN;
    printk("rest page: %d\n", rest_page);

    struct buddy_alloc *buddy = &global_buddy;
    buddy->free_area = __va(start);
    size_t buddy_size = (sizeof(u8) * FREE_AREA_NODE_NUM * free_area_num) +
                        sizeof(struct buddy_alloc);

    buddy->free_area_num = free_area_num;
    buddy->base_addr = __va(start);
    printk("buddy size: %#x\n", buddy_size);

    int node_order = MAX_ORDER + 2;
    for (size_t i = 0; i < free_area_num; i++) {
        for (size_t j = 0; j < FREE_AREA_NODE_NUM; j++) {
            if (is_power_of_2(j + 1)) {
                node_order--;
            }
            *buddy_node(buddy, i, j) = node_order;
        }
        node_order = MAX_ORDER + 2;
    }

    size_t reserve_num = buddy->free_area_num;
    size_t alloc_num = 0;

    // mark reserve
    size_t leaf_index = (1 << MAX_ORDER) - 1;
    size_t free_area_index = 0;
    while (reserve_num) {
        if (leaf_index > FREE_AREA_NODE_NUM) {
            leaf_index = (1 << MAX_ORDER) - 1;
            free_area_index++;
        }
        *buddy_node(buddy, free_area_index, leaf_index) = 0;
        leaf_index++;
        reserve_num--;
    }
    return buddy;
}

#define LEFT_LEAF(index) ((index)*2 + 1)
#define RIGHT_LEAF(index) ((index)*2 + 2)
#define PARENT(index) (((index) + 1) / 2 - 1)

void *buddy_alloc(struct buddy_alloc *buddy, size_t order) {
    if (!buddy || order > MAX_ORDER)
        return NULL;

    size_t i;
    size_t index = 0;
    size_t node_order;
    for (i = 0; i < buddy->free_area_num; i++) {
        if (*buddy_node(buddy, i, 0) >= order + 1) {
            break;
        }
    }

    if (i == buddy->free_area_num) {
        return NULL;
    }

    for (node_order = MAX_ORDER + 1; node_order != order + 1; node_order--) {
        if (*buddy_node(buddy, i, LEFT_LEAF(index)) >= order + 1) {
            index = LEFT_LEAF(index);
        } else {
            index = RIGHT_LEAF(index);
        }
    }
    *buddy_node(buddy, i, index) = 0;
    // TODO: set offset
    void *offset = buddy->base_addr + ((FREE_AREA_PFN * i) << PAGE_SHIFT) +
                   (((index + 1) * (1 << order) - FREE_AREA_PFN) << PAGE_SHIFT);

    printk("alloc order %d, free area %d, index %d, addr %p\n", order, i, index,
           offset);

    while (index) {
        index = PARENT(index);
        *buddy_node(buddy, i, index) =
            max(*buddy_node(buddy, i, LEFT_LEAF(index)),
                *buddy_node(buddy, i, RIGHT_LEAF(index)));
    }

    return offset;
}

void buddy_free(struct buddy_alloc *buddy, void *offset) {
    if (!is_align((unsigned long)offset, PAGE_SIZE)) {
        // ERROR
        return;
    }
    // TODO: use offset compute index and free area index;
    size_t page_offset = ((offset - buddy->base_addr) >> PAGE_SHIFT);
    size_t free_area_index = page_offset / FREE_AREA_PFN;
    size_t index = (page_offset % FREE_AREA_PFN) + FREE_AREA_PFN - 1;

    size_t node_order = 1;
    for (; *buddy_node(buddy, free_area_index, index); index = PARENT(index)) {
        node_order++;
        if (index == 0) {
            break;
        }
    }

    *buddy_node(buddy, free_area_index, index) = node_order;
    printk("free order %d, free area %d, index %d, addr %p", --node_order,
           free_area_index, index, offset);

    while (index) {
        index = PARENT(index);
        node_order++;
        size_t left_order =
            *buddy_node(buddy, free_area_index, LEFT_LEAF(index));
        size_t right_order =
            *buddy_node(buddy, free_area_index, RIGHT_LEAF(index));
        if (left_order + right_order == node_order) {
            *buddy_node(buddy, free_area_index, index) = node_order;
        } else {
            *buddy_node(buddy, free_area_index, index) =
                max(left_order, right_order);
        }
    }
}

size_t buddy_size(struct buddy_alloc *buddy, void *offset) {
    size_t node_order = 1;
    // TODO: use offset compute index;
    size_t page_offset = ((offset - buddy->base_addr) >> PAGE_SHIFT);
    size_t free_area_index = page_offset / FREE_AREA_PFN;
    size_t index = (page_offset % FREE_AREA_PFN) + FREE_AREA_PFN - 1;

    for (; *buddy_node(buddy, free_area_index, index); index = PARENT(index))
        node_order++;
    return 1 << --node_order << PAGE_SHIFT;
}
