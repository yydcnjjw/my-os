#include <kernel/printk.h>

static char buf[1024];

int printk(const char *fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    va_end(args);
    early_vga_write(buf);
    return i;
}

extern void print_banner() { printk("my-os\n"); }
    
