#include <my-os/stack_alloc.h>

void init_stack_pool(struct stack_pool *pool) {
    if (!pool)
        return;

    for (size_t i = 0; i < pool->n_block; ++i) {
        pool->blocks[i] = pool->memory_pool + i * pool->block_size;
    }
}

void *stack_alloc(struct stack_pool *pool) {
    if (!pool)
        return NULL;

    return pool->blocks[--pool->top];
}

void stack_release(struct stack_pool *pool, void *p) {
    if (!pool)
        return;
    if (pool->top == 0) {
        return;
    }
    pool->blocks[pool->top++] = p;
}
