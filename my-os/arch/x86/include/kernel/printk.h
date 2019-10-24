#ifndef _KERNEL_X86_PRINTK_H
#define _KERNEL_X86_PRINTK_H

void early_serial_write(const char *);
unsigned int strlen(const char *);
int early_serial_putc(unsigned char);

#endif /* _KERNEL_X86_PRINTK_H */
