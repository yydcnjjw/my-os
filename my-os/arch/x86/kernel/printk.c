#include <stdarg.h>

#include <kernel/printk.h>

extern void early_serial_write(const char *s);
extern int vsprintf(char * buf, const char * fmt, va_list args);
static char buf[1024];

int printk(const char *fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    va_end(args);
    early_serial_write(buf);
    return i;
}

extern void print_banner() { printk("my-os\n"); }
    
