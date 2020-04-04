#pragma once

#include <my-os/types.h>

#define __force

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define barrier() __asm__ __volatile__("" : : : "memory")

static __always_inline void __write_once_size(volatile void *p, void *res,
                                              int size) {
    switch (size) {
    case 1:
        *(volatile u8 *)p = *(u8 *)res;
        break;
    case 2:
        *(volatile u16 *)p = *(u16 *)res;
        break;
    case 4:
        *(volatile u32 *)p = *(u32 *)res;
        break;
    case 8:
        *(volatile u64 *)p = *(u64 *)res;
        break;
    default:
        barrier();
        __builtin_memcpy((void *)p, (const void *)res, size);
        barrier();
    }
}

#define WRITE_ONCE(x, val)                                                     \
    ({                                                                         \
        union {                                                                \
            typeof(x) __val;                                                   \
            char __c[1];                                                       \
        } __u = {.__val = (__force typeof(x))(val)};                           \
        __write_once_size(&(x), __u.__c, sizeof(x));                           \
        __u.__val;                                                             \
    })
