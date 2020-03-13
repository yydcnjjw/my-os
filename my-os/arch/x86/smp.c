#include <asm/apic.h>
#include <asm/msr.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/smp.h>

#include <kernel/printk.h>
#include <my-os/slub_alloc.h>
#include <my-os/string.h>
#include <my-os/task.h>
#include <my-os/types.h>

extern char smp_boot_start[];
extern char smp_boot_end[];

#define SMP_BOOT_BASE 0x9f000

struct real_mode_data {
    u32 text_start;
    u32 pgd;
};

extern struct real_mode_data real_mode_data;
void smp_boot();
void smp_init(void) {
    unsigned int eax, ebx, ecx, edx;
    int count = 0;
    while (true) {
        cpuid_count(0xb, count, &eax, &ebx, &ecx, &edx);
        u8 level_type = (eax >> 8UL) & 0xff;
        printk("eax %#x\n", eax);
        u8 bits = eax & 0xf;
        if (level_type == 1) { /* SMT */
            printk("SMT bit %d\n", bits);
        } else if (level_type == 2) { /* Core */
            printk("Core bit %d\n", bits);
        } else {
            break;
        }
        count++;
    }
    printk("x2apic id level: %d\n", ecx & 0xff);

    size_t code_size = smp_boot_end - smp_boot_start;
    if (code_size > PTE_SIZE) {
        printk("smp code overflow %d\n", code_size);
        return;
    }

    // TODO: assert init_mm.top_page < 32bit
    // 临时设置 低地址映射
    init_mm.top_page[0] = ((pml4e_t *)__va(early_pml4t))[0];
    
    printk("top page %p\n", init_mm.top_page);

    real_mode_data.text_start = SMP_BOOT_BASE;
    real_mode_data.pgd = (phys_addr_t)init_mm.top_page;

    void *smp_boot_base = __va(real_mode_data.text_start);
    memcpy(smp_boot_base, smp_boot_start, code_size);

#define ICR_OFFSET 0x300
    u32 *icr = __va(LAPIC_DEFAULT_BASE + ICR_OFFSET);
    printk("icr addr %p\n", icr);

    union thread_union *ap_thread_union =
        kmalloc(sizeof(union thread_union), SLUB_NONE);
    if (!ap_thread_union) {

        return;
    }
    extern phys_addr_t initial_stack;
    extern phys_addr_t initial_code;
    printk("stack %p, code %p\n", initial_stack, initial_code);
    initial_stack = (phys_addr_t)ap_thread_union + THREAD_SIZE;
    initial_code = (phys_addr_t)smp_boot;
    printk("stack %p, code %p\n", initial_stack, initial_code);

    printk("smp boot start ...\n");
    *icr = 0xc4500;
    *icr = 0xc469f;
    *icr = 0xc469f;
}

void smp_boot() {
    for (;;)
        ;
}
