#include <kernel/mm.h>
#include <kernel/printk.h>
#include <my-os/buddy_alloc.h>
#include <my-os/kernel.h>
#include <my-os/log2.h>
#include <my-os/mm_types.h>
#include <my-os/slub_alloc.h>
#include <my-os/string.h>

#define ARCH_KMALLOC_MINALIGN __alignof__(unsigned long long)
#define ARCH_SLUB_MINALIGN __alignof__(unsigned long long)

#define PAGE_ALLOC_COSTLY_ORDER 3
#define MAX_OBJS_PER_PAGE 32767 /* since page.objects is u15 */

enum slab_state {
    DOWN,         /* No slab functionality yet */
    PARTIAL,      /* SLUB: kmem_cache_node available */
    PARTIAL_NODE, /* SLAB: kmalloc size for node struct available */
    UP,           /* Slab caches usable but not all extras yet */
    FULL          /* Everything is working */
};
enum slab_state slab_state;
LIST_HEAD(slab_caches);
struct kmem_cache *kmem_cache;
static unsigned int slub_min_order;
static unsigned int slub_max_order = PAGE_ALLOC_COSTLY_ORDER;
static unsigned int slub_min_objects;

static inline unsigned int order_objects(unsigned int order,
                                         unsigned int size) {
    return ((unsigned int)PAGE_SIZE << order) / size;
}

static inline unsigned int slab_order(unsigned int size,
                                      unsigned int min_objects,
                                      unsigned int max_order,
                                      unsigned int fract_leftover) {
    unsigned int min_order = slub_min_order;
    unsigned int order;

    if (order_objects(min_order, size) > MAX_OBJS_PER_PAGE)
        return get_order(size * MAX_OBJS_PER_PAGE) - 1;

    for (order = max(min_order, (unsigned int)get_order(min_objects * size));
         order <= max_order; order++) {

        unsigned int slab_size = (unsigned int)PAGE_SIZE << order;
        unsigned int rem;

        rem = slab_size % size;

        if (rem <= slab_size / fract_leftover)
            break;
    }

    return order;
}

static inline int calculate_order(unsigned int size) {
    unsigned int order;
    unsigned int min_objects;
    unsigned int max_objects;

    min_objects = slub_min_objects;
    if (!min_objects)
        min_objects = 4;
    max_objects = order_objects(slub_max_order, size);
    min_objects = min(min_objects, max_objects);

    while (min_objects > 1) {
        unsigned int fraction;

        fraction = 16;
        while (fraction >= 4) {
            order = slab_order(size, min_objects, slub_max_order, fraction);
            if (order <= slub_max_order)
                return order;
            fraction /= 2;
        }
        min_objects--;
    }

    order = slab_order(size, 1, slub_max_order, 1);
    if (order <= slub_max_order)
        return order;

    order = slab_order(size, 1, MAX_ORDER, 1);
    if (order < MAX_ORDER)
        return order;
    return -1;
}

static int calculate_sizes(struct kmem_cache *s, int forced_order) {
    slub_flags_t flags = s->flags;
    unsigned int size = s->object_size;
    unsigned int order;

    size = ALIGN(size, sizeof(void *));
    s->inuse = size;

    size = ALIGN(size, s->align);
    s->size = size;
    if (forced_order >= 0)
        order = forced_order;
    else
        order = calculate_order(size);

    if ((int)order < 0)
        return 0;

    return 1;
}

static void init_kmem_cache_node(struct kmem_cache_node *n) {
    n->nr_partial = 0;
    INIT_LIST_HEAD(&n->partial);
}

static int kmem_cache_open(struct kmem_cache *s, slub_flags_t flags) {
    if (!calculate_sizes(s, -1))
        goto error;
    init_kmem_cache_node(&s->node);
error:
    return -1;
}

int __kmem_cache_create(struct kmem_cache *s, slub_flags_t flags) {
    int err;

    err = kmem_cache_open(s, flags);
    if (err)
        return err;

    /* Mutex is not taken during early boot */
    if (slab_state <= UP)
        return 0;
    return 0;
}

static unsigned int calculate_alignment(unsigned int align) {
    if (align < ARCH_SLUB_MINALIGN)
        align = ARCH_SLUB_MINALIGN;

    return ALIGN(align, sizeof(void *));
}

void create_boot_cache(struct kmem_cache *s, const char *name,
                       unsigned int size, slub_flags_t flags) {
    int err;

    s->name = name;
    s->size = s->object_size = size;
    unsigned int align = ARCH_KMALLOC_MINALIGN;

    if (is_power_of_2(size)) {
        align = max(align, size);
    }
    s->align = calculate_alignment(align);

    err = __kmem_cache_create(s, flags);

    if (err) {
        printk("create kmalloc slab %s size=%d\n", name, size);
    }

    s->refcount = -1;
}

static inline void *get_freepointer(struct kmem_cache *s, void *object) {
    return (void *)*(unsigned long *)(object + s->offset);
}

static inline void set_freepointer(struct kmem_cache *s, void *object,
                                   void *fp) {
    unsigned long freeptr_addr = (unsigned long)object + s->offset;
    *(void **)freeptr_addr = fp;
}

static inline struct page *alloc_slab_page(struct kmem_cache *s, gfp_t flags) {
    return alloc_pages(s->order);
}

void *page_addr(struct page *page) {
    return __va(page_to_pfn(page) << PAGE_SHIFT);
}

static struct page *new_slab(struct kmem_cache *s, gfp_t flags) {
    struct page *page = alloc_slab_page(s, flags);
    void *start = page_addr(page);
    void *p, *next;
    int idx;
    page->objects = order_objects(s->order, s->size);
    page->freelist = start;
    for (idx = 0, p = start; idx < page->objects; idx++) {
        next = p + s->size;
        set_freepointer(s, p, next);
    }
    set_freepointer(s, p, NULL);
    page->inuse = page->objects;
    page->frozen = 1;
    return page;
}

static void *new_slub_objects(struct kmem_cache *s, gfp_t flags,
                              struct kmem_cache_cpu *c) {
    void *freelist = NULL;
    struct page *page = new_slab(s, flags);
    if (page) {
        freelist = page->freelist;
        page->freelist = NULL;
        c->page = page;
    }
    return freelist;
}

void *_slub_alloc(struct kmem_cache *s, gfp_t gfpflags,
                  struct kmem_cache_cpu *c) {
    void *freelist;
    struct page *page = c->page;
    if (!page)
        goto new_slab;

redo:

    freelist = page->freelist;
    if (!freelist) {
        c->page = NULL;
        goto new_slab;
    }

load_freelist:

    c->freelist = get_freepointer(s, freelist);
    return freelist;

new_slab:
    if (c->partial) {
        page = c->page = c->partial;
        c->partial = page->next;
        goto redo;
    }

    freelist = new_slub_objects(s, gfpflags, c);
    if (!freelist) {
        // out of memory
        return NULL;
    }

    page = c->page;
    goto load_freelist;
}

void *slub_alloc(struct kmem_cache *s, gfp_t gfpflags) {
    if (!s) {
        return NULL;
    }

    struct kmem_cache_cpu *c = &s->cpu_slab;
    void *object = c->freelist;
    if (!object) {
        object = _slub_alloc(s, gfpflags, c);
    } else {
        void *next_object = get_freepointer(s, object);
        c->freelist = next_object;
    }
    return object;
}

void _slub_free(struct kmem_cache *s, struct page *page, void *head) {
    
}

void slub_free(struct kmem_cache *s, struct page *page, void *head) {
    struct kmem_cache_cpu *c = &s->cpu_slab;
    if (c->page == page) {
        set_freepointer(s, head, c->freelist);
        c->freelist = head;
    } else {
        _slub_free(s, page, head);
    }
}

// only for init
static struct kmem_cache *bootstrap(struct kmem_cache *static_cache) {
    struct kmem_cache *s = slub_alloc(kmem_cache, SLUB_NONE);
    memcpy(s, static_cache, kmem_cache->object_size);
    list_add(&s->list, &slab_caches);
    return s;
}

void print_kmem_cache(struct kmem_cache *s) {
    printk("keme cache %s: \n", s->name);
    printk("\tobject size: %#x\n", s->object_size);
    printk("\tsize: %#x\n", s->size);
    printk("\torder: %d\n", s->order);
    printk("\talign: %#x\n", s->align);
}

void kmem_cache_init(void) {
    // only for init
    static struct kmem_cache boot_kmem_cache;

    kmem_cache = &boot_kmem_cache;

    slab_state = PARTIAL;
    create_boot_cache(kmem_cache, "kmem_cache", sizeof(struct kmem_cache),
                      SLUB_NONE);

    print_kmem_cache(kmem_cache);

    kmem_cache = bootstrap(&boot_kmem_cache);
}
