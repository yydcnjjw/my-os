#ifndef _ASM_X86_IDT_H
#define _ASM_X86_IDT_H

struct IDT_entry {
    unsigned short offset_lowbits;
    unsigned short selector;
    unsigned char ist;
    unsigned char type_attr;
    unsigned short offset_middlebits;
    unsigned int offset_highbits;
    unsigned int reserved;
} __attribute__((packed));

struct IDT_ptr {
    unsigned short size;
    unsigned long address;
} __attribute__((packed));

extern void init_idt();

extern struct IDT_entry IDT[];
extern struct IDT_ptr idt_ptr;

#endif /* _ASM_X86_IDT_H */
