#ifndef _MY_OS_KERNEL_H
#define _MY_OS_KERNEL_H

#include <stddef.h>

#define __cmp(x, y, op) ((x)op(y) ? (x) : (y))

#define min(x, y) __cmp(x, y, <)
#define max(x, y) __cmp(x, y, >)

#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        void *__mptr = (void *)(ptr);                                          \
        ((type *)(__mptr - offsetof(type, member)));                           \
    })

#endif /* _MY_OS_KERNEL_H */
