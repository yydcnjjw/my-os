#include <asm/desc.h>
#include <asm/io.h>
#include <asm/msr.h>
#include <asm/page.h>
#include <asm/segment.h>

#include <kernel/printk.h>
#include <my-os/types.h>

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

struct idt_data {
    unsigned int vector;
    unsigned int segment;
    struct idt_bits bits;
    const void *addr;
};

#define DPL0 0x0
#define DPL3 0x3

#define DEFAULT_STACK 0

#define G(_vector, _addr, _ist, _type, _dpl, _segment)                         \
    {                                                                          \
        .vector = _vector, .bits.ist = _ist, .bits.type = _type,               \
        .bits.dpl = _dpl, .bits.p = 1, .addr = _addr, .segment = _segment,     \
    }

/* Interrupt gate */
#define INTG(_vector, _addr)                                                   \
    G(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL0, __KERNEL_CS)

/* System interrupt gate */
#define SYSG(_vector, _addr)                                                   \
    G(_vector, _addr, DEFAULT_STACK, GATE_INTERRUPT, DPL3, __KERNEL_CS)

/*
 * Interrupt gate with interrupt stack. The _ist index is the index in
 * the tss.ist[] array, but for the descriptor it needs to start at 1.
 */
#define ISTG(_vector, _addr, _ist)                                             \
    G(_vector, _addr, _ist + 1, GATE_INTERRUPT, DPL0, __KERNEL_CS)

/* Task gate */
#define TSKG(_vector, _gdt)                                                    \
    G(_vector, NULL, DEFAULT_STACK, GATE_TASK, DPL0, _gdt << 3)

gate_desc IDT[IDT_ENTRIES];

/* struct desc_ptr idt_ptr = { */
/*     .size = (NUM_EXCEPTION_VECTORS * 2 * sizeof(unsigned long)) - 1, */
/*     .address = (unsigned long)IDT}; */

struct desc_ptr idt_ptr = {.size =
                               (IDT_ENTRIES * 2 * sizeof(unsigned long)) - 1,
                           .address = (unsigned long)IDT};

extern const char early_idt_handler_array[NUM_EXCEPTION_VECTORS]
                                         [EARLY_IDT_HANDLER_SIZE];

static inline void load_idt(const struct desc_ptr *dtr) {
    asm volatile("lidt %0" ::"m"(*dtr));
}

extern void int33(void);

void early_init_idt() {
    for (int i = 0; i < NUM_EXCEPTION_VECTORS; i++) {
        unsigned long irq_addr = (unsigned long)early_idt_handler_array[i];
        gate_desc *idt_e = IDT + i;
        idt_e->offset_low = (u16)irq_addr;
        idt_e->segment = (u16)__KERNEL_CS;
        idt_e->bits.type = GATE_INTERRUPT;
        idt_e->bits.p = 1;
        idt_e->offset_middle = (u32)(irq_addr >> 16);
        idt_e->offset_high = (u32)(irq_addr >> 32);
        idt_e->reserved = 0;
    }

    load_idt(&idt_ptr);
}

void idt_setup(void) {
    // debug keyboard
    gate_desc *idt_e = IDT + 0x21;
    unsigned long irq_addr = (unsigned long)int33;
    idt_e->offset_low = (u16)irq_addr;
    idt_e->segment = (u16)__KERNEL_CS;
    idt_e->bits.type = GATE_INTERRUPT;
    idt_e->bits.p = 1;
    idt_e->offset_middle = (u32)(irq_addr >> 16);
    idt_e->offset_high = (u32)(irq_addr >> 32);
    idt_e->reserved = 0;
}

static void print_idt(int vector_num, struct pt_regs *regs) {
    printk("idt handler:\n");
    printk("\tvector num: %#x\n", vector_num);
    printk("ip=%p\n", regs->ip);
    printk("cs=%p\n", regs->cs);
    printk("cr2=%p\n", regs->orig_ax);
}

/* Interrupts/Exceptions */
enum {
    X86_TRAP_DE = 0,    /*  0, Divide-by-zero */
    X86_TRAP_DB,        /*  1, Debug */
    X86_TRAP_NMI,       /*  2, Non-maskable Interrupt */
    X86_TRAP_BP,        /*  3, Breakpoint */
    X86_TRAP_OF,        /*  4, Overflow */
    X86_TRAP_BR,        /*  5, Bound Range Exceeded */
    X86_TRAP_UD,        /*  6, Invalid Opcode */
    X86_TRAP_NM,        /*  7, Device Not Available */
    X86_TRAP_DF,        /*  8, Double Fault */
    X86_TRAP_OLD_MF,    /*  9, Coprocessor Segment Overrun */
    X86_TRAP_TS,        /* 10, Invalid TSS */
    X86_TRAP_NP,        /* 11, Segment Not Present */
    X86_TRAP_SS,        /* 12, Stack Segment Fault */
    X86_TRAP_GP,        /* 13, General Protection Fault */
    X86_TRAP_PF,        /* 14, Page Fault */
    X86_TRAP_SPURIOUS,  /* 15, Spurious Interrupt */
    X86_TRAP_MF,        /* 16, x87 Floating-Point Exception */
    X86_TRAP_AC,        /* 17, Alignment Check */
    X86_TRAP_MC,        /* 18, Machine Check */
    X86_TRAP_XF,        /* 19, SIMD Floating-Point Exception */
    X86_TRAP_IRET = 32, /* 32, IRET Exception */
};

static inline void halt(void) { asm volatile("hlt" : : : "memory"); }

unsigned int early_recursion_flag;

extern void early_fixup_exception(struct pt_regs *regs, int trapnr) {

    if (trapnr == X86_TRAP_NMI)
        return;

    if (early_recursion_flag > 2)
        goto halt_loop;

    printk("recursion flag = %d\n", early_recursion_flag);
    if (trapnr < 32) {
        print_idt(trapnr, regs);
    } else {
        printk("vector num must < 256: %d", trapnr);
    }

halt_loop:
    while (true)
        halt();
}
