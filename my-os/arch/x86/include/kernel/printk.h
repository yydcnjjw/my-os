#ifndef _KERNEL_X86_PRINTK_H
#define _KERNEL_X86_PRINTK_H

void early_serial_init();

int printk(const char *fmt, ...);
unsigned int strlen(const char *);

void print_banner();

#endif /* _KERNEL_X86_PRINTK_H */
