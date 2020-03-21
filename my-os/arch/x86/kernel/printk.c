#include <kernel/printk.h>

static char buf[1024];

int printk(const char *fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vprintk(fmt, args);
    va_end(args);
    return i;
}

int vprintk(const char *fmt, va_list args) {
    int i = vsprintf(buf, fmt, args);
    early_vga_write(buf);
    early_serial_write(buf);
    return i;
}

extern void print_banner() { printk("my-os\n"); }
