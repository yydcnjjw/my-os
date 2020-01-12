#ifndef _MY_OS_MM_TYPES_H
#define _MY_OS_MM_TYPES_H

#include <asm/page.h>
#include <my-os/list.h>

struct mm_struct {
    pml4e_t *top_page;
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, start_stack;
};

extern struct mm_struct init_mm;

struct page {
    unsigned long flags;
    int _refcount;
};

extern struct page *mem_map;

struct free_area {
    struct list_head free_list;
    unsigned long nr_free;
};

#define MAX_ORDER 11
struct zone {
    struct free_area	free_area[MAX_ORDER];
};

#define MAX_NR_ZONES 4 /* __MAX_NR_ZONES */
struct pglist_data {
    struct zone node_zones[MAX_NR_ZONES];
    int nr_zones;
    unsigned long node_start_pfn;
};

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#endif /* _MY_OS_MM_TYPES_H */
