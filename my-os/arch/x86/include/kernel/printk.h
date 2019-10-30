#ifndef _X86_KERNEL_PRINTK_H
#define _X86_KERNEL_PRINTK_H

void early_serial_init();

int printk(const char *fmt, ...);
unsigned int strlen(const char *);

void print_banner();

#endif /* _X86_KERNEL_PRINTK_H */
