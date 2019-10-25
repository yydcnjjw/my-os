#include <asm/io.h>
#include <kernel/printk.h>

#define VGABASE 0xb8000

static int max_ypos = 25, max_xpos = 80;
static int current_ypos, current_xpos;

extern void early_vga_write(const char *str, unsigned int len) {

    char *vga_base = (char *)VGABASE;

    char c;
    while ((c = *str++) != '\0' && len-- > 0) {
        if (current_ypos >= max_ypos) {
            /* scroll 1 line up */
            int j = 0;
            for (int i = 1; i < max_ypos; i++, j++) {
                for (int k = 0; k < max_xpos; k++) {
                    vga_base[2 * (max_xpos * j + k)] =
                        vga_base[2 * (max_xpos * i + k)];
                }
            }
            for (int i = 0; i < max_xpos; i++) {
                vga_base[2 * (max_xpos * j + i)] = (char)0x720;
            }
            current_ypos = max_ypos - 1;
        }

        if (c == '\n') {
            current_xpos = 0;
            current_ypos++;
        } else if (c != '\r') {
            vga_base[2 * (max_ypos * current_ypos + current_xpos++)] = c;

            if (current_xpos >= max_xpos) {
                current_xpos = 1;
                current_ypos++;
            }
        }
    }
}
/* serial io impl */
#define SERIAL_BASE 0x3f8

#define RBR 0 /* Receiver Buffer */
#define THR 0 /* Transmitter Holding */
#define IER 1 /* Interrupt Enable */
#define IIR 2 /* Interrupt ID */
#define FCR 2 /* FIFO control */
#define LCR 3 /* Line control */
#define MCR 4 /* Modem control */
#define LSR 5 /* Line Status */
#define MSR 6 /* Modem Status */

#define DLL 0 /* Divisor Latch Low */
#define DLH 1 /* Divisor latch High */

/* ref: https://www.lammertbies.nl/comm/info/serial-uart.html */
/* ref: https://wiki.osdev.org/Serial_Ports */

static unsigned int io_serial_in(unsigned long addr, int offset) {
    return inb(addr + offset);
}

static void io_serial_out(unsigned long addr, int offset, int value) {
    outb(value, addr + offset);
}

void early_serial_init() {
    io_serial_out(SERIAL_BASE, IER, 0); /* Disable all interrupts */

    io_serial_out(SERIAL_BASE, LCR,
                  0x80); /* Enable DLAB(set baud rate divisor) */
    io_serial_out(SERIAL_BASE, DLL,
                  0x03); /* Set divisor to 3(lo byte) 38400 baud */
    io_serial_out(SERIAL_BASE, DLH, 0); /*                 (hi byte) */

    io_serial_out(SERIAL_BASE, LCR,
                  0x3); /* 8 bits, no parity, one stop bit(8n1) */
    io_serial_out(SERIAL_BASE, FCR, 0);   /* Disable FIFO */
    io_serial_out(SERIAL_BASE, MCR, 0x3); /* DTR + RTS */
}

static inline void pause(void) { asm volatile("pause" ::: "memory"); }

int early_serial_putc(unsigned char ch) {
#define THR_EMPTY 0x20

    unsigned timeout = 0xffff;
    while ((io_serial_in(SERIAL_BASE, LSR) & THR_EMPTY) == 0 && --timeout)
        pause();

    io_serial_out(SERIAL_BASE, THR, ch);
    return timeout ? 0 : -1;
}

void early_serial_write(const char *s) {
    unsigned int n = strlen(s);
    char c;
    while ((c = *s++) && n-- > 0) {
        if (c == '\n') {
            early_serial_putc('\r');
        }
        early_serial_putc(c);
    }
}
