#define __ASSEMBLY__
#include <asm/desc.h>
#include <asm/segment.h>

    .text
    .code64
    .globl start_64
start_64:
    leaq early_stack + 4096, %rsp
    
    lgdt early_gdt_ptr(%rip)
    /* set up data segments */
    xorl %eax,%eax
    movl %eax,%ds
    movl %eax,%ss
    movl %eax,%es

    movq initial_code(%rip), %rax
    pushq $__KERNEL_CS
    pushq %rax
    lretq
    
    .globl early_idt_handler_array
early_idt_handler_array:
    i = 0
    .rept NUM_EXCEPTION_VECTORS
	.if ((EXCEPTION_ERRCODE_MASK >> i) & 1) == 0
		pushq $0
	.endif
    pushq $i
    jmp early_idt_handler_comm
    i = i + 1
    .fill early_idt_handler_array + i*9 - ., 1, 0xcc
    .endr

early_idt_handler_comm:
    cld
    incl early_recursion_flag(%rip)
    
    pushq %rsi
    movq 8(%rsp), %rsi
    movq %rdi, 8(%rsp)
    pushq %rdx
    pushq %rcx
    pushq %rax
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, %rdi
.extern early_fixup_exception    
    call early_fixup_exception

    decl early_recursion_flag(%rip)
    
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

    addq $8, %rsp
    iretq

    .data    
    .align 16
    .globl early_gdt_descr
early_gdt_ptr:
    .word GDT_ENTRIES * 8 - 1
early_gdt_descr_base:
    .quad gdt_page

initial_code:
    .quad x86_64_start_kernel
