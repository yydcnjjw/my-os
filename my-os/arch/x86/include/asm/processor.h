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

static inline void cpuid_count(unsigned int op, int count, unsigned int *eax,
                               unsigned int *ebx, unsigned int *ecx,
                               unsigned int *edx) {
    *eax = op;
    *ecx = count;
    __cpuid(eax, ebx, ecx, edx);
}

static inline unsigned int cpuid_eax(unsigned int op) {
    unsigned int eax, ebx, ecx, edx;
    cpuid(op, &eax, &ebx, &ecx, &edx);
    return eax;
}

static inline unsigned int cpuid_ebx(unsigned int op) {
    unsigned int eax, ebx, ecx, edx;
    cpuid(op, &eax, &ebx, &ecx, &edx);
    return ebx;
}

static inline unsigned int cpuid_ecx(unsigned int op) {
    unsigned int eax, ebx, ecx, edx;
    cpuid(op, &eax, &ebx, &ecx, &edx);
    return ecx;
}

static inline unsigned int cpuid_edx(unsigned int op) {
    unsigned int eax, ebx, ecx, edx;
    cpuid(op, &eax, &ebx, &ecx, &edx);
    return edx;
}

extern unsigned long __force_order;

static inline void write_cr0(unsigned long val) {
    asm volatile("mov %0,%%cr0" : "+r"(val), "+m"(__force_order));
}

static inline unsigned long read_cr0(void) {
    unsigned long val;
    asm volatile("mov %%cr0,%0\n\t" : "=r"(val), "=m"(__force_order));
    return val;
}

static inline unsigned long read_cr2(void) {
    unsigned long val;
    asm volatile("mov %%cr2,%0\n\t" : "=r"(val), "=m"(__force_order));
    return val;
}

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

static inline void write_cr4(unsigned long val) {
    asm volatile("mov %0,%%cr4" : "+r"(val), "+m"(__force_order));
}

static inline unsigned long read_cr4(void) {
    unsigned long val;
    asm volatile("mov %%cr4,%0\n\t" : "=r"(val), "=m"(__force_order));
    return val;
}

#endif /* _X86_ASM_PROCESSOR_H */
