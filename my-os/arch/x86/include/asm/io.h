#ifndef _ASM_X86_IO_H
#define _ASM_X86_IO_H

#define BUILDIO(bwl, bw, type)                                                 \
    static inline void out##bwl(unsigned type value, int port) {               \
        asm volatile("out" #bwl " %" #bw "0, %w1" : : "a"(value), "Nd"(port)); \
    }                                                                          \
                                                                               \
    static inline unsigned type in##bwl(int port) {                            \
        unsigned type value;                                                   \
        asm volatile("in" #bwl " %w1, %" #bw "0" : "=a"(value) : "Nd"(port));  \
        return value;                                                          \
    }


BUILDIO(b, b, char)
BUILDIO(w, w, short)
BUILDIO(l, , int)

#define inb inb
#define inw inw
#define inl inl

#define outb outb
#define outw outw
#define outl outl

#endif /* _ASM_X86_IO_H */
