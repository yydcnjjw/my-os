#ifndef X86_ASM_MSR_H
#define X86_ASM_MSR_H

#include <my-os/types.h>

static inline u64 __rdmsr(u32 msr) {

    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return low | (u64)high << 32;
}

static inline void __wrmsr(u32 msr, u32 low, u32 high) {
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
}

#define rdmsr(msr, val1, val2)                                                 \
    do {                                                                       \
        u64 __val = __rdmsr((msr));                                            \
        (void)((val1) = (u32)__val);                                           \
        (void)((val2) = (u32)__val >> 32);                                     \
    } while (0)

#define rdmsrl(msr, val) ((val) = __rdmsr((msr)))

#define wrmsr(msr, low, high) __wrmsr(msr, low, high)

#define wrmsrl(msr, val)                                                       \
    __wrmsr((msr), (u32)((u64)(val)), (u32)((u64)(val) >> 32))

#endif /* X86_ASM_MSR_H */
