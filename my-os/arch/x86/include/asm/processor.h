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

#endif /* _X86_ASM_PROCESSOR_H */
