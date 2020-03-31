#define __ASSEMBLY__
#include <asm/irq.h>

    .text
    .code64
.macro PUSH_AND_CLEAR_REGS
	pushq %rdi                  /* pt_regs->di */
	pushq %rsi                  /* pt_regs->si */
	pushq %rdx                  /* pt_regs->dx */
	xorl %edx, %edx             /* nospec   dx */
	pushq %rcx                  /* pt_regs->cx */
	xorl %ecx, %ecx             /* nospec   cx */
	pushq %rax                  /* pt_regs->ax */
	pushq %r8                   /* pt_regs->r8 */
	xorl %r8d, %r8d             /* nospec   r8 */
	pushq %r9                   /* pt_regs->r9 */
	xorl %r9d, %r9d             /* nospec   r9 */
	pushq %r10                  /* pt_regs->r10 */
	xorl %r10d, %r10d           /* nospec   r10 */
	pushq %r11                  /* pt_regs->r11 */
	xorl %r11d, %r11d           /* nospec   r11*/
	pushq %rbx                  /* pt_regs->rbx */
	xorl %ebx, %ebx             /* nospec   rbx*/
	pushq %rbp                  /* pt_regs->rbp */
	xorl %ebp, %ebp             /* nospec   rbp*/
	pushq %r12                  /* pt_regs->r12 */
	xorl %r12d, %r12d           /* nospec   r12*/
	pushq %r13                  /* pt_regs->r13 */
	xorl %r13d, %r13d           /* nospec   r13*/
	pushq %r14                  /* pt_regs->r14 */
	xorl %r14d, %r14d           /* nospec   r14*/
	pushq %r15                  /* pt_regs->r15 */
	xorl %r15d, %r15d           /* nospec   r15*/
.endm

.macro POP_REGS
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rax
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
.endm

.macro idt_handler sym do_sym
    .globl \sym
\sym:  
    cld

    PUSH_AND_CLEAR_REGS
    movq %rsp, %rdi
    call \do_sym
    POP_REGS
    iretq
.endm
    
    .align 8
irq_entries_start:
    .globl irq_entries_start
    vector=FIRST_EXTERNAL_VECTOR
    .rept (NR_VECTORS - FIRST_EXTERNAL_VECTOR)
    pushq $(~vector+0x80)
    jmp common_interrupt
    .align 8
    vector=vector+1
    .endr

common_interrupt:   
    addq $-0x80, (%rsp)         /* Adjust vector to [-256, -1] range */
    PUSH_AND_CLEAR_REGS
    movq %rsp, %rdi
    call do_IRQ
    POP_REGS
    addq $8, %rsp
    iretq
