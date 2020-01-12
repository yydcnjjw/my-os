#ifndef _MY_OS_SLUB_ALLOC_H
#define _MY_OS_SLUB_ALLOC_H

#include <my-os/list.h>
#include <my-os/types.h>

typedef unsigned int gfp_t;
typedef unsigned int slub_flags_t;

struct kmem_cache_node {
    unsigned long nr_partial;
    struct list_head partial;
    u64 nr_slabs;
    u64 total_objects;
};

struct kmem_cache {
    unsigned int size;        /* The size of an object including metadata */
    unsigned int object_size; /* The size of an object without metadata */
    unsigned int offset;
    slub_flags_t flags;
    unsigned int order;
    unsigned int align;
    unsigned int inuse; /* Offset to metadata */
    int refcount;       /* Refcount for slab cache destroy */
    const char *name;
    void (*ctor)(void *);
    struct kmem_cache_node node;
    struct list_head list;
};

void *kmalloc(size_t size, gfp_t flags);
void kmem_cache_init(void);

#define SLUB_NONE 0x0U

#endif /* _MY_OS_SLUB_ALLOC_H */
