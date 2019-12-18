KERNEL_LMA_START equ 0x100000
KERNEL_VMA_START equ 0xffffffff80000000
extern KERNEL_LMA_END

%define pml4e_index(addr) ((addr >> 39) & 0x1ff)
%define pdpte_index(addr) ((addr >> 30) & 0x1ff)
%define MMU_PRESENT (1 << 0)
%define MMU_WRITABLE (1 << 1)
%define MMU_USER_MEMORY (1 << 2)
%define MMU_PDE_TWO_MB (1 << 7)

;; Declare constants for the multiboot2 header.
MB_MAGIC equ 0xe85250d6        ; 'magic number' lets bootloader find the header
MB_ARCH equ 0                  ; 0 i386 4 MIPS
MB_HEADER_LEN equ multiboot_header_end - multiboot_header_start
MB_CHECKSUM equ 0x100000000-(MB_MAGIC + MB_ARCH + MB_HEADER_LEN)

MB_TAG_END equ 0

section .multiboot
align 4
multiboot_header_start:
    dd MB_MAGIC
    dd MB_ARCH
    dd MB_HEADER_LEN
    dd MB_CHECKSUM
    ; tag end
    dw MB_TAG_END
    dw 0
    dd 8
multiboot_header_end:

bits 32
section .boot_text
global start
start:    
    mov esp, boot_stack_top
    ; push multiboot info
    push eax                    ; magic
    push ebx                    ; multiboot_info_t

    ; detection of cpuid
    pushfd
    pop eax

    ; Flip the id bit
    mov ecx, eax
    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    ; CPUID isn't supported
    xor eax, ecx
    jz no_longmode

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_longmode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_longmode

    ; disable paging
    mov eax, cr0
    and eax, ~(1<<31)
    mov cr0, eax

    mov eax, pdpte_index(KERNEL_VMA_START)
    mov eax, pdpte_low
    or eax, MMU_PRESENT | MMU_WRITABLE
    mov dword [pml4t + pml4e_index(KERNEL_LMA_START) * 8], eax

    mov eax, pdpte_high
    or eax, MMU_PRESENT | MMU_WRITABLE
    mov dword [pml4t + pml4e_index(KERNEL_VMA_START) * 8], eax

    mov eax, pde_low
    or eax, MMU_PRESENT | MMU_WRITABLE
    mov dword [pdpte_low + pdpte_index(KERNEL_LMA_START) * 8], eax

    mov eax, pde_high
    or eax, MMU_PRESENT | MMU_WRITABLE
    mov dword [pdpte_high + pdpte_index(KERNEL_VMA_START) * 8], eax

    xor ecx, ecx

    mov esi, KERNEL_LMA_END
    shr esi, 21
    add esi, 1

set_pdt:
    mov eax, 1 << 21
    mul ecx
    or eax, MMU_PRESENT | MMU_WRITABLE | MMU_PDE_TWO_MB
    mov [pde_low + ecx], eax
    mov [pde_high + ecx], eax

    inc ecx
    cmp esi, ecx
    jne set_pdt

    mov eax, pml4t
    mov cr3, eax

    mov eax, cr4
    mov eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    pop ebx
    pop eax

    lgdt [gdt_ptr]
    jmp boot64codeseg_index:boot_start_64

hang:
    cli
    hlt
    jmp hang

no_longmode:
    cli
    hlt
    jmp no_longmode
    
bits 64
boot_start_64:
    mov rdi, rbx
    mov rsi, rax

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
extern start_64
    jmp start_64

section .boot_data
global boot_stack_top
align 16
boot_stack_bottom:
    ; resb 0x4000                 ; 16 KiB
    times 0x4000 db 0
boot_stack_top:

align 0x1000
pml4t:
    times 512 dq 0
pdpte_low:
    times 512 dq 0
pde_low:
    times 512 dq 0
pdpte_high:
    times 512 dq 0
pde_high:
    times 512 dq 0

gdt:
    dw 0, 0
    db 0, 0x00, 0x00, 0
boot64codeseg_index equ $ - gdt
boot64codeseg:
    dw 0xffff, 0
    db 0, 0x9e, 0xaf, 0
gdtlen equ $ - gdt
gdt_ptr:
    dw gdtlen - 1
    dd gdt
