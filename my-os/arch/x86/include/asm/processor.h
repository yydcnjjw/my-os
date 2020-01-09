#ifndef _X86_ASM_PROCESSOR_H
#define _X86_ASM_PROCESSOR_H

static inline void __cpuid(unsigned int *eax, unsigned int *ebx,
                           unsigned int *ecx, unsigned int *edx) {
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "0"(*eax), "2"(*ecx)
                 : "memory");
}

static inline void cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx,
                         unsigned int *ecx, unsigned int *edx) {
    *eax = op;
    *ecx = 0;
    __cpuid(eax, ebx, ecx, edx);
}

extern unsigned long __force_order;
#define CR3_ADDR_MASK 0x7FFFFFFFFFFFF000ull
static inline unsigned long __read_cr3(void) {
    unsigned long val;
    asm volatile("mov %%cr3,%0\n\t" : "=r"(val), "=m"(__force_order));
    return val;
}

static inline void write_cr3(unsigned long x) {
    asm volatile("mov %0,%%cr3" : : "r"(x), "m"(__force_order));
}

static inline unsigned long read_cr3_pa(void) {
    return __read_cr3() & CR3_ADDR_MASK;
}

#endif /* _X86_ASM_PROCESSOR_H */
