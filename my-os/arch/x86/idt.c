#include <asm/desc.h>
#include <asm/idt.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/msr.h>
#include <asm/page.h>
#include <asm/segment.h>

#include <kernel/printk.h>
#include <my-os/kernel.h>
#include <my-os/string.h>
#include <my-os/types.h>

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

gate_desc idt_table[IDT_ENTRIES];

/* struct desc_ptr idt_ptr = { */
/*     .size = (NUM_EXCEPTION_VECTORS * 2 * sizeof(unsigned long)) - 1, */
/*     .address = (unsigned long)IDT}; */

struct desc_ptr idt_ptr = {.size =
                               (IDT_ENTRIES * 2 * sizeof(unsigned long)) - 1,
                           .address = (unsigned long)idt_table};

extern const char early_idt_handler_array[NUM_EXCEPTION_VECTORS]
                                         [EARLY_IDT_HANDLER_SIZE];

static inline void load_idt(const struct desc_ptr *dtr) {
    asm volatile("lidt %0" ::"m"(*dtr));
}

static inline void idt_init_desc(gate_desc *gate, const struct idt_data *d) {
    unsigned long addr = (unsigned long)d->addr;
    gate->offset_low = (u16)addr;
    gate->segment = (u16)d->segment;
    gate->bits = d->bits;
    gate->offset_middle = (u16)(addr >> 16);
    gate->offset_high = (u32)(addr >> 32);
    gate->reserved = 0;
}

static void idt_setup_from_table(gate_desc *idt, const struct idt_data *t,
                                 int size) {
    gate_desc desc;

    for (; size > 0; t++, size--) {
        idt_init_desc(&desc, t);
        memcpy(&idt[t->vector], &desc, sizeof(desc));
    }
}

static void set_intr_gate(unsigned int n, const void *addr) {
    struct idt_data data = INTG(n, addr);
    idt_setup_from_table(idt_table, &data, 1);
}

void early_init_idt() {
    for (int i = 0; i < NUM_EXCEPTION_VECTORS; i++) {
        phys_addr_t irq_addr = (phys_addr_t)early_idt_handler_array[i];
        set_intr_gate(i, (void *)irq_addr);
        /* gate_desc *idt_e = idt_table + i; */
        /* idt_e->offset_low = (u16)irq_addr; */
        /* idt_e->segment = (u16)__KERNEL_CS; */
        /* idt_e->bits.type = GATE_INTERRUPT; */
        /* idt_e->bits.p = 1; */
        /* idt_e->offset_middle = (u32)(irq_addr >> 16); */
        /* idt_e->offset_high = (u32)(irq_addr >> 32); */
        /* idt_e->reserved = 0; */
    }

    load_idt(&idt_ptr);
}

/* typedef void (*irq_handler_t)(void); */
/* void irq_set_handler(u32 irq, irq_handler_t handle) { */
/*     gate_desc *idt_e = idt_table + irq; */
/*     unsigned long irq_addr = (unsigned long)handle; */
/*     idt_e->offset_low = (u16)irq_addr; */
/*     idt_e->segment = (u16)__KERNEL_CS; */
/*     idt_e->bits.type = GATE_INTERRUPT; */
/*     idt_e->bits.p = 1; */
/*     idt_e->offset_middle = (u32)(irq_addr >> 16); */
/*     idt_e->offset_high = (u32)(irq_addr >> 32); */
/*     idt_e->reserved = 0; */
/* } */
extern char irq_entries_start[IRQ_VECTORS][IRQ_ENTRIES_START_SIZE];
void idt_setup(void) {
    for (int i = FIRST_EXTERNAL_VECTOR; i < NR_VECTORS; ++i) {
        set_intr_gate(i, (void *)irq_entries_start[i - FIRST_EXTERNAL_VECTOR]);
    }
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
        halt("idt error");
}
