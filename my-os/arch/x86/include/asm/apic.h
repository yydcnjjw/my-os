#ifndef X86_ASM_APIC_H
#define X86_ASM_APIC_H

#include <my-os/types.h>

#define IOAPIC_DEFAULT_BASE 0xfec00000
#define LAPIC_DEFAULT_BASE 0xfee00000

#define IOAPIC_ID_INDEX 0x00
#define IOAPIC_VERSION_INDEX 0x01

#define IOAPIC_RTE_BASE_INDEX 0x10
#define IOAPIC_RTE_END_INDEX 0x40

#define IOAPIC_RTE_KEYBOARD (IOAPIC_RTE_BASE_INDEX + 2)
#define IOAPIC_RTE_HPET (IOAPIC_RTE_BASE_INDEX + 4)

#define EOI_REG_OFFSET 0xb0


void local_apic_init(void);

void apic_eoi(void);

#endif /* X86_ASM_APIC_H */
