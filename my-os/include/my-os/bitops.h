#ifndef _MY_OS_BITOPS_H
#define _MY_OS_BITOPS_H
#include <my-os/types.h>
#define X86_64

static inline int fls(unsigned int x) {
    int r;
#ifdef X86_64
    asm("bsrl %1,%0" : "=r"(r) : "rm"(x), "0"(-1));
#else
    asm("bsrl %1,%0\n\t"
        "jnz 1f\n\t"
        "movl $-1,%0\n"
        "1:"
        : "=r"(r)
        : "rm"(x));
#endif
    return r + 1;
}

#ifdef X86_64
static inline int fls64(u64 x) {
    int bitpos = -1;
    asm("bsrq %1,%q0" : "+r"(bitpos) : "rm"(x));
    return bitpos + 1;
}
#endif

#endif /* _MY_OS_BITOPS_H */
