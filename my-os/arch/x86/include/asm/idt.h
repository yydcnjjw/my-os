#pragma once

#include <my-os/types.h>

typedef void (*irq_handler_t)(void);
void irq_set_handler(u32 irq, irq_handler_t handle);
