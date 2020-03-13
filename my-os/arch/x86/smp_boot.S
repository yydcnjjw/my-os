#define offset_addr(addr) (addr - smp_boot_start)

    .text
    .balign 0x1000
smp_boot_start:
    .globl smp_boot_start

    .code16
smp_boot_code16:
    cli
    wbinvd
    mov %cs, %ax
    mov %ax, %ds

    // base addr offset
    movl offset_addr(pa_text_start), %esi

    // fix gdt_ptr
    leal offset_addr(gdt)(%esi), %eax
    movl %eax, offset_addr(gdt_ptr) + 0x2

    lidtl offset_addr(smp_tmp_idt)
    lgdtl offset_addr(gdt_ptr)

    smsw %ax
    bts $0, %ax
    lmsw %ax

    // fix addr
    leal offset_addr(smp_boot_code32)(%esi), %eax
    movl %eax, offset_addr(smp_boot_code32_entry)
    leal offset_addr(smp_boot_code64)(%esi), %eax
    movl %eax, offset_addr(smp_boot_code64_entry)

    ljmpl *offset_addr(smp_boot_code32_entry)

    .code32
smp_boot_code32:
    mov $0x08, %ax
    mov %cs, %ax
    mov $0x10, %ax
    mov %ax, %ds

    // leal offset_addr(smp_boot_stack_top)(%esi), %eax
    // mov %eax, %esp

    movl offset_addr(pa_pgd)(%esi), %eax
    movl %eax, %cr3

    // enable PAE
    movl %cr4, %eax
    bts $5, %eax
    movl %eax, %cr4

    movl $0xc0000080, %ecx
    rdmsr
    bts $8, %eax
    wrmsr

    movl %cr0, %eax
    bts $31, %eax
    movl %eax, %cr0

    ljmp *offset_addr(smp_boot_code64_entry)(%esi)

    .code64
smp_boot_code64:
    movq $0x18, %rax
    movq %rax, %ds
    leaq start_64, %rax
    jmpq *%rax

.balign 4
smp_tmp_idt:
    .word 0
    .word 0,0

.balign 4
smp_boot_code32_entry:
    .long 0
    .word 0x08, 0

.balign 4
smp_boot_code64_entry:
    .long 0
    .word 0x18, 0

real_mode_data:
    .globl real_mode_data
pa_text_start:
    .long 0
pa_pgd:
    .long 0

.balign 4
gdt:
    .word 0, 0
    .byte 0, 0x00, 0x00, 0
boot32codeseg:
    .word 0xffff, 0
    .byte 0, 0x9e, 0xcf, 0
boot32dataseg:
    .word 0xffff, 0
    .byte 0, 0x92, 0xcf, 0
boot64codeseg:
    .word 0xffff, 0
    .byte 0, 0x9e, 0xaf, 0
gdt_end:

gdt_ptr:
    .word gdt_end - gdt - 1
    .long 0

smp_boot_stack_bottom:
    .org 0x400
smp_boot_stack_top:

smp_boot_end:
    .globl smp_boot_end
