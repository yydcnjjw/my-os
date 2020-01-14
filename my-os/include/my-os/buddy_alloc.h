#ifndef _MY_OS_BUDDY_ALLOC_H
#define _MY_OS_BUDDY_ALLOC_H

#include <my-os/mm_types.h>
#include <my-os/types.h>

void init_buddy_alloc(void);

phys_addr_t _alloc_pages(size_t order);

struct page *alloc_pages(size_t order);

static inline struct page *alloc_page() { return alloc_pages(0); }

void free_pages(struct page *);

size_t pages_size(struct page *);

#endif /* _MY_OS_BUDDY_ALLOC_H */
