#include <asm/io.h>
#include <asm/irq.h>
#include <kernel/printk.h>
#include <my-os/string.h>

static u8 VGA_HEIGHT = 25, VGA_WIDTH = 80;
static u8 current_col = 0, current_row = 0;

u16 *vga_base = (u16 *)VGA_BASE;
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_MISC_READ 0x3CC
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF

#define VGA_CRTC_INDEX 0x3D4 /* 0x3B4 */
#define VGA_CRTC_DATA 0x3D5  /* 0x3B5 */
#define VGA_INSTAT_READ 0x3DA

#define VGA_NUM_SEQ_REGS 5
#define VGA_NUM_CRTC_REGS 25
#define VGA_NUM_GC_REGS 9
#define VGA_NUM_AC_REGS 21
#define VGA_NUM_REGS                                                           \
    (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + VGA_NUM_GC_REGS +              \
     VGA_NUM_AC_REGS)

void enable_cursor() {
    outb(0x09, VGA_CRTC_INDEX); /* max scan line register */
    /* u8 max_scan_line = inb(VGA_CRTC_DATA) & 0x1ff; */

    outb(0x0a, VGA_CRTC_INDEX);
    outb(inb(VGA_CRTC_DATA) | 0, 0x3d5);

    outb(0x0b, VGA_CRTC_INDEX);
    outb(inb(VGA_CRTC_DATA) | 0x1ff, 0x3d5);
}

void disable_cursor() {
    outb(0x0a, 0x3d4);
    outb(0x20, 0x3d5);
}

// FIXME: col row max value
void update_cursor() {
    if (current_row >= VGA_HEIGHT || current_col >= VGA_WIDTH) {
        return;
    }
    u16 pos = current_row * VGA_WIDTH + current_col;

    outb(0x0f, VGA_CRTC_INDEX);
    outb((u8)(pos & 0xff), VGA_CRTC_DATA);
    outb(0x0e, VGA_CRTC_INDEX);
    outb((u8)((pos >> 8) & 0xff), VGA_CRTC_DATA);
}

void enable_vga() {
    enable_cursor();
    update_cursor();
    current_col = 0;
    current_row = 24;
    update_cursor();
    current_row = 0;
}

u16 *get_vga_ptr(u8 row, u8 col) { return vga_base + VGA_WIDTH * row + col; }

void set_vga_base(void *addr) { vga_base = (u16 *)addr; }

void early_vga_clear_row(u8 row) {
    for (int col = 0; col < VGA_WIDTH; col++) {
        *get_vga_ptr(row, col) = 0x0f00;
    }
}

void early_vga_scroll() {
    /* scroll 1 line up */
    int j = 0;
    for (int row = 1; row <= current_row; row++, j++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            u16 *cover = get_vga_ptr(row - 1, col);
            u16 *value = get_vga_ptr(row, col);
            *cover = *value;
        }
    }

    early_vga_clear_row(current_row);
    current_col = 0;
}

void early_vga_write(const char *str) {
    unsigned int len = strlen(str);
    char c;
    while ((c = *str++) != '\0' && len-- > 0) {
        if (c == '\n') {
            current_col = 0;
            current_row++;
            /* if (current_row == VGA_HEIGHT) { */
            /*     current_row--; */
            /*     early_vga_scroll(); */
            /* } */
        } else if (c != '\r') {
            *get_vga_ptr(current_row, current_col++) |= c;
            if (current_col == VGA_WIDTH) {
                current_col = 0;
                current_row++;
            }
        }
        if (current_row == VGA_HEIGHT) {
            current_row--;
            early_vga_scroll();
        }
    }
    update_cursor();
}

void vga_backspace() {
    /* irq_disable(); */
    if (current_col == 0) {
        if (current_row != 0) {
            current_row--;
            current_col = VGA_WIDTH - 1;
        }
    }
    current_col--;
    *get_vga_ptr(current_row, current_col) = 0x0f00;
    update_cursor();
    /* irq_enable(); */
}

void early_vga_init() {
    /* write_regs(g_80x25_text); */
    enable_vga();

    for (int i = 0; i < VGA_HEIGHT; i++) {
        early_vga_clear_row(i);
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
