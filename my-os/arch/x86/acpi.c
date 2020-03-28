#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/idt.h>
#include <asm/page.h>

#include <kernel/printk.h>
#include <my-os/string.h>

struct RSDTDescriptor *rsdt;
struct address_structure {
    u8 address_space_id; // 0 - system memory, 1 - system I/O
    u8 register_bit_width;
    u8 register_bit_offset;
    u8 reserved;
    u64 base_addr;
} __attribute__((packed));

struct HPET {
    struct SDTHeader header;
    u8 hardware_rev_id;
    u8 comparator_count : 5;
    u8 counter_size : 1;
    u8 reserved : 1;
    u8 legacy_replacement : 1;
    u16 pci_vendor_id;
    struct address_structure address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
} __attribute__((packed));

#define HPET_REG_GENERAL_CAP_ID 0x00
#define HPET_REG_GENERAL_CNF 0x10
#define HPET_REG_MAIN_CNT_VALUE 0xf0

#define HPET_REG_N_TIMER_CNF_CAP(n) (0x100 + 0x20 * n)
#define HPET_REG_N_TIMER_COMP_VAL(n) (0x108 + 0x20 * n)
#define HPET_REG_N_TIMER_FSB_INTER(n) (0x110 + 0x20 * n)

struct HPET_general_cap_id_reg {
    u8 rev_id;
    u8 num_tim_cap : 5;
    u8 counter_size_cap : 1;
    u8 reserved : 1;
    u8 leg_rt_cap : 1;
    u16 vendor_id;
    u32 counter_clk_preiod;
} __attribute__((packed));

struct HPET_timer_conf_cap_reg {
    u16 reserved0 : 1;
    u16 tn_int_type_cnf : 1;
    u16 tn_int_enb_cnf : 1;
    u16 tn_type_cnf : 1;
    u16 tn_per_int_cap : 1;
    u16 tn_size_cap : 1;
    u16 tn_val_set_cnf : 1;
    u16 tn_reserved : 1;
    u16 tn_32mode_cnf : 1;
    u16 tn_int_route_cnf : 5;
    u16 tn_fsb_en_cnf : 1;
    u16 tn_fsb_int_del_cap : 1;
    u16 reserved1;
    u32 tn_int_route_cap;
} __attribute__((packed));

void acpi_init() {
    printk("rsdt addr %p\n", rsdt);
    rsdt = __va(rsdt);
    printk("rsdt va addr %p\n", rsdt);
    int entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
    char buf[5] = {0};
    for (int i = 0; i < entries; i++) {
        struct SDTHeader *sdt = __va(rsdt->other_sdt[i]);
        memcpy(buf, sdt->signature, 4);
        printk("sdt %s\n", buf);
        if (!memcmp(sdt->signature, "HPET", 4)) {
            struct HPET *hpet = (struct HPET *)sdt;
            printk("addr %p\n", hpet->address.base_addr);

            void *base_addr = __va(hpet->address.base_addr);

            u64 *reg;
            reg = base_addr + HPET_REG_GENERAL_CAP_ID;
            struct HPET_general_cap_id_reg cap_reg =
                *(struct HPET_general_cap_id_reg *)reg;

            u32 COUNTER_CLK_PERIOD = cap_reg.counter_clk_preiod;
            printk("counter clk period %d\n", COUNTER_CLK_PERIOD);

            reg = base_addr + HPET_REG_N_TIMER_CNF_CAP(0);
            struct HPET_timer_conf_cap_reg cnf_reg =
                *(struct HPET_timer_conf_cap_reg *)reg;

            cnf_reg.tn_int_enb_cnf = 1;
            cnf_reg.tn_type_cnf = 1;
            cnf_reg.tn_val_set_cnf = 1;
            cnf_reg.tn_int_route_cnf = cnf_reg.tn_int_route_cap;
            *reg = *(u64 *)&cnf_reg;

            reg = base_addr + HPET_REG_N_TIMER_COMP_VAL(0);
            u32 freq = 1e15 / COUNTER_CLK_PERIOD;
            printk("freq %d\n", freq);
            *reg = freq;

            reg = base_addr + HPET_REG_GENERAL_CNF;
#define ENABLE_CNF_BIT 0
#define LEG_RT_CNF_BIT 1
            *reg = 1 << LEG_RT_CNF_BIT | 1 << ENABLE_CNF_BIT;

            extern void int34(void);
            irq_set_handler(0x22, int34);
            wrioapicl(IOAPIC_RTE_HPET, 0x22);
        }
    }
}

void do_timer(struct pt_regs *regs) {
    printk("timer\n");
    u32 *eoi = __va(LAPIC_DEFAULT_BASE + EOI_REG_OFFSET);
    *eoi = 0;
}
