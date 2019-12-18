#include <asm/desc.h>
#include <asm/io.h>
#include <asm/multiboot2/api.h>

#include <kernel/printk.h>
#include <my-os/start_kernel.h>

char early_stack[4096] = {0};

static inline void irq_disable(void) { asm volatile("cli" : : : "memory"); }

static inline void irq_enable(void) { asm volatile("sti" : : : "memory"); }

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define ICW1_ICW4 0x01      /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

void pic_remap(int offset1, int offset2) {
    u8 a1, a2;

    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(ICW1_INIT | ICW1_ICW4, PIC1_COMMAND);
    outb(ICW1_INIT | ICW1_ICW4, PIC2_COMMAND);
    outb(offset1, PIC1_DATA);
    outb(offset2, PIC2_DATA);
    outb(4, PIC1_DATA);
    outb(2, PIC2_DATA);
    outb(ICW4_8086, PIC1_DATA);
    outb(ICW4_8086, PIC2_DATA);
    
    // set mask
    outb(a1, PIC1_DATA);
    outb(a2, PIC2_DATA);
}

void pic_disable() {
    outb(0xff, PIC1_DATA);
    outb(0xff, PIC2_DATA);
}

void x86_64_start_kernel(void *addr, unsigned int magic) {
    early_serial_init();
    early_init_idt();
    pic_remap(0x20, 0x28);
    pic_disable();
    irq_enable();

    if (-1 == parse_multiboot2(addr, magic)) {
        // TODO: panic
    }
    start_kernel();

    while (true)
        ;
}
