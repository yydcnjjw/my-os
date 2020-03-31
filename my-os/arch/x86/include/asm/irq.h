#pragma once

#define FIRST_EXTERNAL_VECTOR 0x20
#define NR_VECTORS 256
#define IRQ_VECTORS (NR_VECTORS - FIRST_EXTERNAL_VECTOR)
#define IRQ_ENTRIES_START_SIZE 8

#define NR_IRQS 256

#ifndef __ASSEMBLY__

#include <my-os/types.h>

enum irqreturn {
    IRQ_NONE = (0 << 0),
    IRQ_HANDLED = (1 << 0),
    IRQ_WAKE_THREAD = (1 << 1),
};

typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);
struct irq_desc;
typedef void (*irq_flow_handler_t)(struct irq_desc *desc);

struct irq_action {
    irq_handler_t handler;
    void *dev_id;
    struct irq_action *next;
    const char *name;
    unsigned int irq;
    unsigned int flags;
};

struct irq_data {
    unsigned int irq;
};

struct irq_desc {
    struct irq_data irq_data;
    irq_flow_handler_t handle_irq;
    struct irq_action *action;
    const char *name;
};

typedef struct irq_desc *vector_irq_t[NR_VECTORS];

extern struct irq_desc irq_desc[NR_IRQS];
static inline struct irq_desc *irq_to_desc(unsigned int irq) {
    return (irq < NR_IRQS) ? irq_desc + irq : NULL;
}

void handle_bad_irq(struct irq_desc *desc);
void handle_simple_irq(struct irq_desc *desc);
void irq_set_handler(unsigned int irq, irq_flow_handler_t handle,
                     const char *name);

void setup_irq(int irq, struct irq_action *new);

void init_IRQ(void);

#endif
