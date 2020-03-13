#include <asm/apic.h>
#include <asm/multiboot2/api.h>
#include <asm/page_types.h>
#include <asm/processor.h>
#include <asm/sections.h>
#include <asm/smp.h>

#include <my-os/buddy_alloc.h>
#include <my-os/memblock.h>
#include <my-os/mm_types.h>
#include <my-os/slub_alloc.h>

#include <kernel/mm.h>
#include <kernel/printk.h>

#define CPUID_FEAT_EDX_FPU 0
#define CPUID_FEAT_EDX_MMX 23
#define CPUID_FEAT_EDX_FXSR 24

#define __AC(X, Y) (X##Y)
#define _AC(X, Y) __AC(X, Y)
#define _AT(T, X) ((T)(X))

#define _UL(x) (_AC(x, UL))
#define _ULL(x) (_AC(x, ULL))
#define _BITUL(x) (_UL(1) << (x))
#define _BITULL(x) (_ULL(1) << (x))

#define X86_CR0_EM_BIT 2 /* Emulation */
#define X86_CR0_EM _BITUL(X86_CR0_EM_BIT)
#define X86_CR0_TS_BIT 3 /* Task Switched */
#define X86_CR0_TS _BITUL(X86_CR0_TS_BIT)
#define X86_CR4_OSFXSR_BIT 9 /* enable fast FPU save and restore */
#define X86_CR4_OSFXSR _BITUL(X86_CR4_OSFXSR_BIT)
#define X86_CR4_OSXMMEXCPT_BIT 10 /* enable unmasked SSE exceptions */
#define X86_CR4_OSXMMEXCPT _BITUL(X86_CR4_OSXMMEXCPT_BIT)
void fpu_init() {

    unsigned long cr4 = read_cr4();
    bool has_fxsr = cpuid_edx(0x80000001) & 1 << CPUID_FEAT_EDX_FXSR;

    if (has_fxsr) {
        cr4 |= X86_CR4_OSFXSR;
    }
    bool has_mmx = cpuid_edx(0x80000001) & 1 << CPUID_FEAT_EDX_MMX;
    if (has_mmx) {
        cr4 |= X86_CR4_OSXMMEXCPT;
    }
    write_cr4(cr4);

    unsigned long cr0 = read_cr0();
    cr0 &= ~(X86_CR0_TS | X86_CR0_EM);
    bool has_fpu = cpuid_edx(0x80000001) & 1 << CPUID_FEAT_EDX_FPU;
    if (!has_fpu) {
        cr0 |= X86_CR0_EM;
        printk("no fpu\n");
    }
    write_cr0(cr0);
    asm volatile("fninit");

    float a = 1.0f;
    printk("%d\n", (int)(a * 10 * 2.1));
}

size_t end_pfn;
void start_kernel(void) {

    printk("lma end %p\n", (unsigned long)KERNEL_LMA_END);

    init_mm.start_code = (unsigned long)_text;
    init_mm.end_code = (unsigned long)_etext;
    init_mm.start_data = (unsigned long)_data;
    init_mm.end_data = (unsigned long)_edata;
    init_mm.start_brk = (unsigned long)_brk_base;

    printk("start code = %p\n", init_mm.start_code);
    printk("end code = %p\n", init_mm.end_code);
    printk("start data = %p\n", init_mm.start_data);
    printk("end data = %p\n", init_mm.end_data);
    printk("start brk = %p\n", init_mm.start_brk);

    memblock_init();
    early_alloc_pgt_buf();

    end_pfn = multiboot2_end_of_ram_pfn();

    multiboot2_memblock_setup();
    print_memblock();

    init_mem_mapping();
    set_vga_base(__va(VGA_BASE));
    init_buddy_alloc();

    mem_init();

    kmem_cache_init();

    void *p[10];
    for (int i = 0; i < 10; i++) {
        p[i] = kmalloc(8, SLUB_NONE);
    }

    for (int i = 0; i < 5; i++) {
        kfree(p[i]);
    }
    for (int i = 0; i < 5; i++) {
        p[i] = kmalloc(8, SLUB_NONE);
    }
    for (int i = 0; i < 10; i++) {
        kfree(p[i]);
    }

    local_apic_init();

    extern void idt_setup(void);
    idt_setup();

    smp_init();

    fpu_init();

    for (;;)
        ;
}
