#ifndef _MY_OS_STACK_ALLOC_H
#define _MY_OS_STACK_ALLOC_H

#include <my-os/types.h>

// usage:
// STACK_POOL(name, num, type);
// init_stack_pool(pool);

struct stack_pool {
    void *memory_pool;
    void **blocks;
    size_t top;
    size_t n_block;
    size_t block_size;
};

#define STACK_POOL_INIT(pool_ptr, blocks_ptr, num, type)                       \
    {                                                                          \
        .memory_pool = pool_ptr, .blocks = blocks_ptr, .top = num,             \
        .n_block = num, .block_size = sizeof(type)                             \
    }

#define STACK_POOL(name, num, type)                                            \
    char __##name##_memory[(num) * sizeof(type)];                              \
    void *__##name##_blocks[num];                                              \
    struct stack_pool name =                                                   \
        STACK_POOL_INIT(__##name##_memory, __##name##_blocks, num, type)

void init_stack_pool(struct stack_pool *pool);

void *stack_alloc(struct stack_pool *pool);

void stack_release(struct stack_pool *pool, void *p);

#endif /* _MY_OS_STACK_ALLOC_H */
