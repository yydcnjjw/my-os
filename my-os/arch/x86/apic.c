#include <asm/msr.h>
#include <asm/processor.h>
#include <kernel/printk.h>
#include <my-os/types.h>

#define CPUID_FEAT_EDX_APIC 0x9

#define MSR_IA32_APICBASE 0x0000001b
#define MSR_IA32_APICBASE_BSP (1 << 8)
#define MSR_IA32_APICBASE_ENABLE (1 << 11)
#define MSR_IA32_APICBASE_BASE (~(0xfffUL))

bool check_apic() {
    unsigned int eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    return edx & 1 << CPUID_FEAT_EDX_APIC;
}

u64 enable_apic() {
    u64 val;
    rdmsrl(MSR_IA32_APICBASE, val);
    val |= MSR_IA32_APICBASE_ENABLE;
    wrmsrl(MSR_IA32_APICBASE, val);
    return val;
}

void local_apic_init(void) {

    if (check_apic()) {
        printk("apic available\n");
    }

    printk("apic base %p\n", enable_apic() & MSR_IA32_APICBASE_BASE);    
}
