#include <my-os/kernel.h>
#include <my-os/log2.h>
#include <my-os/mm_types.h>
#include <my-os/slub_alloc.h>

#include <kernel/printk.h>

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

struct kmem_cache *kmem_cache;
static struct kmem_cache *kmem_cache_node;

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

static int kmem_cache_open(struct kmem_cache *s, slub_flags_t flags) {
    if (!calculate_sizes(s, -1))
        goto error;
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

// only for init
static struct kmem_cache * bootstrap(struct kmem_cache *static_cache) {
    
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
    static struct kmem_cache boot_kmem_cache, boot_kmem_cache_node;

    kmem_cache_node = &boot_kmem_cache_node;
    kmem_cache = &boot_kmem_cache;
    create_boot_cache(kmem_cache_node, "kmem_cache_node",
                      sizeof(struct kmem_cache_node), SLUB_NONE);

    slab_state = PARTIAL;
    create_boot_cache(kmem_cache, "kmem_cache", sizeof(struct kmem_cache),
                      SLUB_NONE);

    print_kmem_cache(kmem_cache);
    print_kmem_cache(kmem_cache_node);

	kmem_cache = bootstrap(&boot_kmem_cache);
	kmem_cache_node = bootstrap(&boot_kmem_cache_node);
}
