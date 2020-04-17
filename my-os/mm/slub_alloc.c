#include <kernel/mm.h>
#include <kernel/printk.h>
#include <my-os/buddy_alloc.h>
#include <my-os/kernel.h>
#include <my-os/log2.h>
#include <my-os/mm_types.h>
#include <my-os/slub_alloc.h>
#include <my-os/string.h>
#include <asm/irq.h>

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
    s->inuse = 0;

    size = ALIGN(size, s->align);
    s->size = size;
    if (forced_order >= 0)
        order = forced_order;
    else
        order = calculate_order(size);

    if ((int)order < 0)
        return 0;

    return size;
}

static void init_kmem_cache_node(struct kmem_cache_node *n) {
    n->nr_partial = 0;
    INIT_LIST_HEAD(&n->partial);
}

static int kmem_cache_open(struct kmem_cache *s, slub_flags_t flags) {
    if (!calculate_sizes(s, -1))
        goto error;
    init_kmem_cache_node(&s->node);
    return 0;
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
        printk("panic %d create kmalloc slab %s size=%d\n", err, name, size);
    }
    printk("create kmalloc slab %s size=%d\n", name, size);
    s->refcount = 0;
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
    for (idx = 0, p = start; idx < page->objects - 1; idx++) {
        next = p + s->size;
        set_freepointer(s, p, next);
        p = next;
    }
    set_freepointer(s, p, NULL);
    page->inuse = 0;
    page->frozen = 1;
    page->slub_cache = s;
    return page;
}

static void add_partial(struct kmem_cache *s, gfp_t gfpflags,
                        struct page *page) {
    s->node.nr_partial++;
    list_add(&page->slub_list, &s->node.partial);
}

static void remove_partial(struct kmem_cache *s, gfp_t gfpflags,
                           struct page *page) {
    s->node.nr_partial--;
    list_del(&page->slub_list);
    free_pages(page);

    struct kmem_cache_cpu *c = &s->cpu_slab;
    if (c->page == page) {
        c->page = NULL;
        c->freelist = NULL;
    }
    if (!list_empty(&s->node.partial)) {
        int min = ((1 << 16) - 1);
        struct page *p;
        struct page *min_page;
        list_for_each_entry(p, &s->node.partial, slub_list) {
            if (p->inuse < min) {
                min = p->inuse;
                min_page = p;
            }
        }
        c->partial = min_page;
    }
}

static struct page *new_slub_objects(struct kmem_cache *s, gfp_t flags) {
    struct page *page = new_slab(s, flags);
    if (page) {
        add_partial(s, flags, page);
        return page;
    }
    return NULL;
}

void *_slub_alloc(struct kmem_cache *s, gfp_t gfpflags,
                  struct kmem_cache_cpu *c) {
    void *freelist;
    struct page *page = c->page;
    if (!page)
        goto new_slab;

redo:

    freelist = page->freelist;
    page->freelist = NULL;
    if (!freelist) {
        c->page = NULL;
        goto new_slab;
    }

    c->freelist = get_freepointer(s, freelist);
    return freelist;

new_slab:
    if (c->partial) {
        c->page = c->partial;

        c->partial = list_next_entry(c->partial, slub_list);
        if ((void *)c->partial == &s->node) {
            c->partial = NULL;
        }

        goto redo;
    }

    page = new_slub_objects(s, gfpflags);
    if (!page) {
        // out of memory
        return NULL;
    }
    c->partial = page;
    goto new_slab;
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
    if (object) {
        c->page->inuse++;
        s->inuse++;
        bzero(object, s->size);
    }
    return object;
}

void _slub_free(struct kmem_cache *s, struct page *page, void *head) {
    struct page *p;
    list_for_each_entry(p, &s->node.partial, slub_list) {
        if (page == p) {
            break;
        }
    }
    set_freepointer(s, head, p->freelist);
    p->freelist = head;
}

void slub_free(struct kmem_cache *s, gfp_t flags, struct page *page,
               void *head) {

#ifdef SLUB_DEBUG
    printk("slub free %s size %#x page %p addr %p\n", s->name, s->size, page,
           head);
#endif
    struct kmem_cache_cpu *c = &s->cpu_slab;
    if (c->page == page) {
        set_freepointer(s, head, c->freelist);
        c->freelist = head;
    } else {
        _slub_free(s, page, head);
    }
    page->inuse--;
    if (page->inuse == 0) {
        remove_partial(s, flags, page);
    }
}

void kfree(void *addr) {
    if (!addr) {
        return;
    }

    struct page *page = virt_to_page(addr);

    if (page->slub_cache) {
        slub_free(page->slub_cache, SLUB_NONE, page, addr);
    } else {
        free_pages(page);
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

#define KMALLOC_SHIFT_LOW 3
#define KMALLOC_MIN_SIZE (1 << KMALLOC_SHIFT_LOW)
#define KMALLOC_SHIFT_HIGH (PAGE_SHIFT + 1)
#define KMALLOC_MAX_CACHE_SIZE (1UL << KMALLOC_SHIFT_HIGH)

struct kmem_cache *kmalloc_caches[KMALLOC_SHIFT_HIGH + 1];

static u8 size_index[24] = {
    3, /* 8 */
    4, /* 16 */
    5, /* 24 */
    5, /* 32 */
    6, /* 40 */
    6, /* 48 */
    6, /* 56 */
    6, /* 64 */
    1, /* 72 */
    1, /* 80 */
    1, /* 88 */
    1, /* 96 */
    7, /* 104 */
    7, /* 112 */
    7, /* 120 */
    7, /* 128 */
    2, /* 136 */
    2, /* 144 */
    2, /* 152 */
    2, /* 160 */
    2, /* 168 */
    2, /* 176 */
    2, /* 184 */
    2  /* 192 */
};

static inline unsigned int size_index_elem(unsigned int bytes) {
    return (bytes - 1) / 8;
}

struct kmem_cache *kmalloc_slub(size_t size, gfp_t flags) {
    unsigned int index;

    if (size <= 192) {
        if (!size)
            return NULL;

        index = size_index[size_index_elem(size)];
    } else {
        if (size > KMALLOC_MAX_CACHE_SIZE)
            return NULL;
        index = fls(size - 1);
    }

    return kmalloc_caches[index];
}

size_t slub_ksize(const struct kmem_cache *s) { return s->object_size; }

size_t ksize(const void *ptr) {
    struct page *page = virt_to_page(ptr);
    if (!page->slub_cache) {
        return pages_size(page);
    }
    return slub_ksize(page->slub_cache);
}

static void *kmalloc_large(size_t size, gfp_t flags) {
    int order = get_order(size);
    struct page *page = alloc_pages(order);
    void *p = page_addr(page);
#ifdef SLUB_DEBUG
    printk("alloc large size %#x addr %p\n", 1 << order, p);
#endif
    return p;
}

void *kmalloc(size_t size, gfp_t flags) {
    struct kmem_cache *s;
    // Consider the case is larger than the cache
    if (size > KMALLOC_MAX_CACHE_SIZE) {
        return kmalloc_large(size, flags);
    }
    s = kmalloc_slub(size, flags);
    if (!s) {
        return s;
    }
    void *o = slub_alloc(s, flags);
#ifdef SLUB_DEBUG
    printk("slub alloc %s size %#x addr %p\n", s->name, s->size, o);
#endif
    return o;
}

// TODO:
void *krealloc(void *p, size_t size, gfp_t flags) {

    size_t ks = 0;
    if (p) {
        ks = ksize(p);
    }

    if (ks >= size) {
        return p;
    }

    void *new = kmalloc(size, flags);
    if (new &&p) {
        memcpy(new, p, size);
    }

    return new;
}

struct kmem_cache *create_kmalloc_cache(const char *name, unsigned int size,
                                        slub_flags_t flags) {
    struct kmem_cache *s = slub_alloc(kmem_cache, flags);
    printk("slub alloc %s: %p\n", kmem_cache->name, s);
    if (!s) {
        // panic
        printk("panic out of memory when creating slub");
        return NULL;
    }

    create_boot_cache(s, name, size, flags);
    list_add(&s->list, &slab_caches);
    s->refcount = 1;
    return s;
}
struct kmalloc_info_t {
    const char *name;
    unsigned int size;
};

#define INIT_KMALLOC_INFO(__size, __short_size)                                \
    { .name = "kmalloc-" #__short_size, .size = __size, }

const struct kmalloc_info_t kmalloc_info[] = {
    INIT_KMALLOC_INFO(0, 0),     INIT_KMALLOC_INFO(96, 96),
    INIT_KMALLOC_INFO(192, 192), INIT_KMALLOC_INFO(8, 8),
    INIT_KMALLOC_INFO(16, 16),   INIT_KMALLOC_INFO(32, 32),
    INIT_KMALLOC_INFO(64, 64),   INIT_KMALLOC_INFO(128, 128),
    INIT_KMALLOC_INFO(256, 256), INIT_KMALLOC_INFO(512, 512),
    INIT_KMALLOC_INFO(1024, 1k), INIT_KMALLOC_INFO(2048, 2k),
    INIT_KMALLOC_INFO(4096, 4k), INIT_KMALLOC_INFO(8192, 8k)};

// only for init
void create_kmalloc_caches(slub_flags_t flags) {
    for (int i = 1; i < KMALLOC_SHIFT_HIGH + 1; i++) {
        kmalloc_caches[i] = create_kmalloc_cache(kmalloc_info[i].name,
                                                 kmalloc_info[i].size, flags);
    }
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
    create_kmalloc_caches(SLUB_NONE);
}
