#pragma once

#include <asm/idt.h>

void divide_error(void);                /* #DE */
void debug(void);                       /* #DB */
void nmi(void);                         /* NMI */
void int3(void);                        /* #BP */
void overflow(void);                    /* #OF */
void bounds(void);                      /* #BR */
void invalid_op(void);                  /* #UD */
void device_not_available(void);        /* #NM */
void double_fault(void);                /* #DF */
void coprocessor_segment_overrun(void); /*  */
void invalid_TSS(void);                 /* #TS */
void segment_not_present(void);         /* #NP */
void stack_segment(void);               /* #SS */
void general_protection(void);          /* #GP */
void page_fault(void);                  /* #PF */
void alignment_check(void);             /* #AC */
void machine_check(void);               /* #MC */
void simd_coprocessor_error(void);      /* #XM */

void do_divide_error(struct pt_regs *regs, long error_code);         /* #DE */
void do_debug(struct pt_regs *regs, long error_code);                /* #DB */
void do_nmi(struct pt_regs *regs, long error_code);                  /*  */
void do_int3(struct pt_regs *regs, long error_code);                 /* #BP */
void do_overflow(struct pt_regs *regs, long error_code);             /* #OF */
void do_bounds(struct pt_regs *regs, long error_code);               /* #BR */
void do_invalid_op(struct pt_regs *regs, long error_code);           /* #UD */
void do_device_not_available(struct pt_regs *regs, long error_code); /* #NM */
void do_double_fault(struct pt_regs *regs, long error_code);         /* #DF */
void do_coprocessor_segment_overrun(struct pt_regs *regs,
                                    long error_code);                  /*  */
void do_invalid_TSS(struct pt_regs *regs, long error_code);            /* #TS */
void do_segment_not_present(struct pt_regs *regs, long error_code);    /* #NP */
void do_stack_segment(struct pt_regs *regs, long error_code);          /* #SS */
void do_general_protection(struct pt_regs *regs, long error_code);     /* #GP */
void do_page_fault(struct pt_regs *regs, long error_code);             /* #PF */
void do_alignment_check(struct pt_regs *regs, long error_code);        /* #AC */
void do_machine_check(struct pt_regs *regs, long error_code);          /* #MC */
void do_simd_coprocessor_error(struct pt_regs *regs, long error_code); /* #XM */
