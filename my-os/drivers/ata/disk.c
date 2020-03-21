#include <asm/idt.h>
#include <asm/io.h>

extern void int2f(void);
void ata_init() {
    irq_set_handler(0x2f, int2f);
    
}

void do_ata(struct pt_regs *regs) {}
