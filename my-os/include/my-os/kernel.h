#ifndef _MY_OS_KERNEL_H
#define _MY_OS_KERNEL_H
#include <kernel/printk.h>
#include <my-os/types.h>
#include <stddef.h>

#define __cmp(x, y, op) ((x)op(y) ? (x) : (y))

#define min(x, y) __cmp(x, y, <)
#define max(x, y) __cmp(x, y, >)

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y)) + 1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        void *__mptr = (void *)(ptr);                                          \
        ((type *)(__mptr - offsetof(type, member)));                           \
    })

static inline __attribute__((const)) bool is_power_of_2(unsigned long n) {
    return (n != 0 && ((n & (n - 1)) == 0));
}

#define __ALIGN_KERNEL(x, a) __ALIGN_KERNEL_MASK(x, (typeof(x))(a)-1)
#define __ALIGN_KERNEL_MASK(x, mask) (((x) + (mask)) & ~(mask))
/* @a is a power of 2 value */
#define ALIGN(x, a) __ALIGN_KERNEL((x), (a))
#define IS_ALIGNED(x, a) (((x) & ((typeof(x))(a)-1)) == 0)

static inline void halt() { asm volatile("cli;hlt" : : : "memory"); }

#define kassert(cond)                                                          \
    do {                                                                       \
        if (!(cond))                                                           \
            halt();                                                            \
    } while (0)

#define __AC(X, Y) (X##Y)
#define _AC(X, Y) __AC(X, Y)
#define _AT(T, X) ((T)(X))

#define _UL(x) (_AC(x, UL))
#define _ULL(x) (_AC(x, ULL))
#define _BITUL(x) (_UL(1) << (x))
#define _BITULL(x) (_ULL(1) << (x))

#endif /* _MY_OS_KERNEL_H */
