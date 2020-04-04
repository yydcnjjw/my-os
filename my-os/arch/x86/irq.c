#include <asm/irq.h>

#include <asm/apic.h>
#include <asm/idt.h>
#include <asm/page.h>
#include <kernel/printk.h>
#include <my-os/kernel.h>

struct irq_desc irq_desc[NR_IRQS] = {
    [0 ... NR_IRQS - 1] = {.handle_irq = handle_bad_irq}};

#define VECTOR_UNUSED NULL
vector_irq_t vector_irq = {[0 ... NR_VECTORS - 1] = VECTOR_UNUSED};

void desc_set_defaults(int i, struct irq_desc *desc) {
    desc->irq_data.irq = i;
    desc->action = NULL;
}

#define ISA_IRQ_VECTOR(irq) (((FIRST_EXTERNAL_VECTOR + 16) & ~15) + irq)
#define NR_LEGACY_IRQS 16
void init_IRQ(void) {
    for (int i = 0; i < NR_VECTORS; i++) {
        desc_set_defaults(i, irq_desc + i);
    }

    for (int i = 0; i < NR_LEGACY_IRQS; i++) {
        vector_irq[0x20 + i] = irq_to_desc(i);
    }
}

void setup_irq(int irq, struct irq_action *new) {
    struct irq_desc *desc = irq_to_desc(irq);
    new->irq = irq;

    struct irq_action **p, *old;

    p = &desc->action;

    if ((old = *p) != NULL) {
        do {
            p = &old->next;
            old = *p;
        } while (old);
    }
    *p = new;
}

#define for_each_action_of_desc(desc, act)                                     \
    for (act = desc->action; act; act = act->next)

irqreturn_t handle_irq_event(struct irq_desc *desc) {
    irqreturn_t retval = IRQ_NONE;
    int irq = desc->irq_data.irq;

    struct irq_action *action;
    for_each_action_of_desc(desc, action) {
        irqreturn_t res;
        res = action->handler(irq, action->dev_id);
        retval |= res;
    }
    return retval;
}

void handle_bad_irq(struct irq_desc *desc) {
    printk("bad irq %d", desc->irq_data.irq);
    apic_eoi();
}

void handle_simple_irq(struct irq_desc *desc) {
    handle_irq_event(desc);
    apic_eoi();
}

void irq_set_handler(unsigned int irq, irq_flow_handler_t handle,
                     const char *name) {
    struct irq_desc *desc = irq_to_desc(irq);
    if (!handle) {
        desc->handle_irq = handle_bad_irq;
    } else {
        desc->handle_irq = handle;
        desc->name = name;
    }
}

void do_IRQ(struct pt_regs *regs) {
    unsigned vector = ~regs->orig_ax;

    struct irq_desc *desc = vector_irq[vector];
    if (desc) {
        desc->handle_irq(desc);
    } else {
        apic_eoi();
        printk("vector number %d no irq handler \n", vector);
    }
}
