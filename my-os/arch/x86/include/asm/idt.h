#pragma once

#ifndef __ASSEMBLY__
#include <my-os/types.h>
#include <asm/irq.h>
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

void idt_setup(void);
#endif

#define R15 0 * 8
#define R14 1 * 8
#define R13 2 * 8
#define R12 3 * 8
#define RBP 4 * 8
#define RBX 5 * 8
/* These regs are callee-clobbered. Always saved on kernel entry. */
#define R11 6 * 8
#define R10 7 * 8
#define R9 8 * 8
#define R8 9 * 8
#define RAX 10 * 8
#define RCX 11 * 8
#define RDX 12 * 8
#define RSI 13 * 8
#define RDI 14 * 8
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
#define ORIG_RAX 15 * 8
/* Return frame for iretq */
#define RIP 16 * 8
#define CS 17 * 8
#define EFLAGS 18 * 8
#define RSP 19 * 8
#define SS 20 * 8

#define SIZEOF_PTREGS 21 * 8
