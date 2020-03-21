#ifndef _X86_KERNEL_PRINTK_H
#define _X86_KERNEL_PRINTK_H

#include <stdarg.h>

void early_serial_init();
void early_serial_write(const char *s);

void early_vga_init();
void early_vga_write(const char *s);
void vga_backspace();
    
void set_vga_base(void *addr);

int printk(const char *fmt, ...);
int vprintk(const char *fmt, va_list args);
int vsprintf(char * buf, const char * fmt, va_list args);
void print_banner();


#define VGA_BASE 0xb8000

#endif /* _X86_KERNEL_PRINTK_H */
