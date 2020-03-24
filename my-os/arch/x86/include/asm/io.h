#ifndef _X86_ASM_IO_H
#define _X86_ASM_IO_H

#define BUILDIO(bwl, bw, type)                                                 \
    static inline void out##bwl(unsigned type value, int port) {               \
        asm volatile("out" #bwl " %" #bw "0, %w1" : : "a"(value), "Nd"(port)); \
    }                                                                          \
                                                                               \
    static inline unsigned type in##bwl(int port) {                            \
        unsigned type value;                                                   \
        asm volatile("in" #bwl " %w1, %" #bw "0" : "=a"(value) : "Nd"(port));  \
        return value;                                                          \
    }                                                                          \
    static inline void outs##bwl(int port, const void *addr,                   \
                                 unsigned long count) {                        \
        asm volatile("rep; outs" #bwl                                          \
                     : "+S"(addr), "+c"(count)                                 \
                     : "d"(port)                                               \
                     : "memory");                                              \
    }                                                                          \
                                                                               \
    static inline void ins##bwl(int port, void *addr, unsigned long count) {   \
        asm volatile("rep; ins" #bwl                                           \
                     : "+D"(addr), "+c"(count)                                 \
                     : "d"(port)                                               \
                     : "memory");                                              \
    }

BUILDIO(b, b, char)
BUILDIO(w, w, short)
BUILDIO(l, , int)

#define inb inb
#define inw inw
#define inl inl
#define insb insb
#define insw insw
#define insl insl

#define outb outb
#define outw outw
#define outl outl
#define outsb outsb
#define outsw outsw
#define outsl outsl

#endif /* _X86_ASM_IO_H */
