#ifndef _MY_OS_BUDDY_ALLOC_H
#define _MY_OS_BUDDY_ALLOC_H
#include <my-os/types.h>
struct buddy_alloc *buddy_new(phys_addr_t start, phys_addr_t end);
void *buddy_alloc(struct buddy_alloc *buddy, size_t order);
int buddy_free(struct buddy_alloc *buddy, void *offset);
size_t buddy_size(struct buddy_alloc *buddy, void *offset);
#endif /* _MY_OS_BUDDY_ALLOC_H */
