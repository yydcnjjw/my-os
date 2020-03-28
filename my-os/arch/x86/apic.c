#include <asm/apic.h>
#include <asm/msr.h>
#include <asm/page.h>
#include <asm/processor.h>

#include <kernel/printk.h>
#include <my-os/types.h>

#define CPUID_FEAT_EDX_APIC 0x9
#define CPUID_FEAT_EDX_X2APIC 0x21

#define MSR_IA32_APICBASE 0x0000001b
#define MSR_IA32_APICBASE_BSP (1 << 8)
#define MSR_IA32_APICBASE_ENABLE (1 << 11)
/* #define MSR_IA32_X2APIC_ENABLE (1 << 10) */
#define MSR_IA32_APICBASE_BASE (~(0xfffUL))

#define MSR_X2APIC_SIVR 0x0000080f

struct IOAPIC_map {
    phys_addr_t addr;
    u8 *index;
    u32 *data;
    u32 *eoi;
};

bool check_apic() {
    unsigned int eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    return edx & 1 << CPUID_FEAT_EDX_APIC & 1 << CPUID_FEAT_EDX_APIC;
}

u64 enable_apic() {
    u64 val;
    rdmsrl(MSR_IA32_APICBASE, val);
    val |= MSR_IA32_APICBASE_ENABLE;
    wrmsrl(MSR_IA32_APICBASE, val);
    return val;
}

#define IOAPIC_BASE (IOAPIC_DEFAULT_BASE + PAGE_OFFSET)
struct IOAPIC_map ioapic = {.addr = IOAPIC_BASE,
                            .index = (void *)IOAPIC_BASE,
                            .data = (void *)(IOAPIC_BASE + 0x10),
                            .eoi = (void *)(IOAPIC_BASE + 0x40)};

u64 rdioapicl(u8 index) {
    u64 ret;
    *ioapic.index = index + 1;
    ret = *ioapic.data;
    ret <<= 32;
    *ioapic.index = index;
    ret |= *ioapic.data;
    return ret;
}

u32 rdioapic(u8 index) {
    *ioapic.index = index;
    return *ioapic.data;
}

void wrioapicl(u8 index, u64 value) {
    *ioapic.index = index;
    *ioapic.data = value & 0xffffffff;
    value >>= 32;
    *ioapic.index = index + 1;
    *ioapic.data = value & 0xffffffff;
}

void wrioapic(u8 index, u32 value) {
    *ioapic.index = index;
    *ioapic.data = value;
}

void ioapic_init(void) {

    u32 ioapic_id = rdioapic(IOAPIC_ID_INDEX);
    printk("%#x\n", ioapic_id);
    u32 ioapic_version = rdioapic(IOAPIC_VERSION_INDEX);
    printk("%#x\n", ioapic_version);

    for (int i = IOAPIC_RTE_BASE_INDEX; i < IOAPIC_RTE_END_INDEX; i += 2) {
        // ignore all
        wrioapicl(i, 0x10020 + ((i - 0x10) >> 1));
    }

    // enable keyboard
    wrioapicl(IOAPIC_RTE_KEYBOARD, 0x21);
    // enable hpet
    /* wrioapicl(IOAPIC_RTE_HPET, 0x22); */
}

void local_apic_init(void) {

    if (check_apic()) {
        printk("apic available\n");
    }

    printk("apic base %p\n", enable_apic() & MSR_IA32_APICBASE_BASE);
    /* u64 sivr; */
    /* rdmsrl(MSR_X2APIC_SIVR, sivr); */
    /* printk("%#x\n", sivr); */
    /* sivr |=  */
    /* wrmsrl(MSR_X2APIC_SIVR, sivr); */

    ioapic_init();
}
