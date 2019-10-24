#include "idt.h"

#include <kernel/printk.h>

#define IDT_ENTRIES 256
#define NUM_EXCEPTION_VECTORS 32
struct IDT_entry IDT[IDT_ENTRIES];

struct IDT_ptr idt_ptr = {
    .size = (NUM_EXCEPTION_VECTORS * 2 * sizeof(unsigned long)) - 1,
    .address = (unsigned long)IDT};

extern const char early_idt_handler_array[NUM_EXCEPTION_VECTORS][8];

void init_idt() {
    for (int i = 0; i < NUM_EXCEPTION_VECTORS; i++) {
        unsigned long irq_addr = (unsigned long)early_idt_handler_array[i];
        struct IDT_entry *idt_e = IDT + i;
        idt_e->offset_lowbits = irq_addr;
        idt_e->offset_middlebits = irq_addr >> 16;
        idt_e->offset_highbits = irq_addr >> 32;
        idt_e->selector = 0x08;
        idt_e->ist = 0;
        idt_e->type_attr = 0x8e;
        idt_e->reserved = 0;
    }
    extern void load_idt(struct IDT_ptr *);
    load_idt(&idt_ptr);
}

struct pt_regs {
    /*
     * C ABI says these regs are callee-preserved. They aren't saved on kernel
     * entry unless syscall needs a complete, fully filled "struct pt_regs".
     */
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long bp;
    unsigned long bx;
    /* These regs are callee-clobbered. Always saved on kernel entry. */
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long ax;
    unsigned long cx;
    unsigned long dx;
    unsigned long si;
    unsigned long di;
    /*
     * On syscall entry, this is syscall#. On CPU exception, this is error code.
     * On hw interrupt, it's IRQ number:
     */
    unsigned long orig_ax;
    /* Return frame for iretq */
    unsigned long ip;
    unsigned long cs;
    unsigned long flags;
    unsigned long sp;
    unsigned long ss;
    /* top of stack page */
};

static void print_num(int num) {
#define BITS 4    
    int bits = BITS - 1;    
    char s_vector_num[BITS] = {};
    int n;
    while (bits >= 0 || num != 0) {
        n = num % 10;
        s_vector_num[bits] = n + '0';
        num = num / 10;
        bits--;
    }
    for (int i = 0; i < BITS; i++) {
        early_serial_putc(s_vector_num[i]);
    }    
}

static void print_idt(int vector_num) {
    early_serial_write("idt handler:\n\tvector num: ");
    print_num(vector_num);
    early_serial_putc('\n');
}

extern void early_fixup_exception(struct pt_regs *regs, int trapnr) {
    if (trapnr < 256) {
        print_idt(trapnr);
    } else {
        early_serial_write("vector num must < 256: ");
        print_num(trapnr);
        early_serial_write("\n");
    }
}
