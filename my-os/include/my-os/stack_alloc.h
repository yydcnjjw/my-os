#ifndef _MY_OS_STACK_ALLOC_H
#define _MY_OS_STACK_ALLOC_H

#include <my-os/types.h>

struct stack_pool {
    void *base;
    void *top;
    size_t size;
};

#define STACK_POOL_INIT(base, size)                                            \
    { .base = base, .size = size, .top = 0 }

#define STACK_POOL(name, size)                                                 \
    char *__##name##_pool[size];                                                 \
    struct stack_pool name = STACK_POOL_INIT(__##name_pool, size)

static inline void *stack_alloc(struct stack_pool *stack, size_t size) {
    if (stack->base + stack->size < stack->top + size) {
        return NULL;
    }
    void *ret = stack->top;
    stack->top += size;
    return ret;
}

static inline void stack_release(struct stack_pool *stack, size_t size) {
    if (stack->base > stack->top - size) {
        return;
    }
    stack->top -= size;
}

#endif /* _MY_OS_STACK_ALLOC_H */
