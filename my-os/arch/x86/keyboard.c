#include <asm/apic.h>
#include <asm/idt.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/page.h>

#include <kernel/printk.h>
#include <my-os/slub_alloc.h>
#include <my-os/types.h>

struct pt_regs;

#define NR_SCAN_CODES 0x80
#define MAP_COLS 2

#define PAUSEBREAK 1
#define PRINTSCREEN 2
#define OTHERKEY 4
#define FLAG_BREAK 0x80

int shift_l = 0, shift_r = 0, ctrl_l = 0, ctrl_r = 0, alt_l = 0, alt_r = 0;

unsigned char pausebreak_scode[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};

unsigned int keycode_map_normal[NR_SCAN_CODES * MAP_COLS] = //
    {
        /*scan-code	unShift		Shift		*/
        /*--------------------------------------------------------------*/
        /*0x00*/ 0,    0,
        /*0x01*/ 0,    0, // ESC
        /*0x02*/ '1',  '!',
        /*0x03*/ '2',  '@',
        /*0x04*/ '3',  '#',
        /*0x05*/ '4',  '$',
        /*0x06*/ '5',  '%',
        /*0x07*/ '6',  '^',
        /*0x08*/ '7',  '&',
        /*0x09*/ '8',  '*',
        /*0x0a*/ '9',  '(',
        /*0x0b*/ '0',  ')',
        /*0x0c*/ '-',  '_',
        /*0x0d*/ '=',  '+',
        /*0x0e*/ '\b', 0, // BACKSPACE
        /*0x0f*/ '\t', 0, // TAB

        /*0x10*/ 'q',  'Q',
        /*0x11*/ 'w',  'W',
        /*0x12*/ 'e',  'E',
        /*0x13*/ 'r',  'R',
        /*0x14*/ 't',  'T',
        /*0x15*/ 'y',  'Y',
        /*0x16*/ 'u',  'U',
        /*0x17*/ 'i',  'I',
        /*0x18*/ 'o',  'O',
        /*0x19*/ 'p',  'P',
        /*0x1a*/ '[',  '{',
        /*0x1b*/ ']',  '}',
        /*0x1c*/ '\n', 0,    // ENTER
        /*0x1d*/ 0x1d, 0x1d, // CTRL Left
        /*0x1e*/ 'a',  'A',
        /*0x1f*/ 's',  'S',

        /*0x20*/ 'd',  'D',
        /*0x21*/ 'f',  'F',
        /*0x22*/ 'g',  'G',
        /*0x23*/ 'h',  'H',
        /*0x24*/ 'j',  'J',
        /*0x25*/ 'k',  'K',
        /*0x26*/ 'l',  'L',
        /*0x27*/ ';',  ':',
        /*0x28*/ '\'', '"',
        /*0x29*/ '`',  '~',
        /*0x2a*/ 0x2a, 0x2a, // SHIFT Left
        /*0x2b*/ '\\', '|',
        /*0x2c*/ 'z',  'Z',
        /*0x2d*/ 'x',  'X',
        /*0x2e*/ 'c',  'C',
        /*0x2f*/ 'v',  'V',

        /*0x30*/ 'b',  'B',
        /*0x31*/ 'n',  'N',
        /*0x32*/ 'm',  'M',
        /*0x33*/ ',',  '<',
        /*0x34*/ '.',  '>',
        /*0x35*/ '/',  '?',
        /*0x36*/ 0x36, 0x36, // SHIFT Right
        /*0x37*/ '*',  '*',
        /*0x38*/ 0x38, 0x38, // ALT Left
        /*0x39*/ ' ',  ' ',
        /*0x3a*/ 0,    0, // CAPS LOCK
        /*0x3b*/ 0,    0, // F1
        /*0x3c*/ 0,    0, // F2
        /*0x3d*/ 0,    0, // F3
        /*0x3e*/ 0,    0, // F4
        /*0x3f*/ 0,    0, // F5

        /*0x40*/ 0,    0, // F6
        /*0x41*/ 0,    0, // F7
        /*0x42*/ 0,    0, // F8
        /*0x43*/ 0,    0, // F9
        /*0x44*/ 0,    0, // F10
        /*0x45*/ 0,    0, // NUM LOCK
        /*0x46*/ 0,    0, // SCROLL LOCK
        /*0x47*/ '7',  0, /*PAD HONE*/
        /*0x48*/ '8',  0, /*PAD UP*/
        /*0x49*/ '9',  0, /*PAD PAGEUP*/
        /*0x4a*/ '-',  0, /*PAD MINUS*/
        /*0x4b*/ '4',  0, /*PAD LEFT*/
        /*0x4c*/ '5',  0, /*PAD MID*/
        /*0x4d*/ '6',  0, /*PAD RIGHT*/
        /*0x4e*/ '+',  0, /*PAD PLUS*/
        /*0x4f*/ '1',  0, /*PAD END*/

        /*0x50*/ '2',  0, /*PAD DOWN*/
        /*0x51*/ '3',  0, /*PAD PAGEDOWN*/
        /*0x52*/ '0',  0, /*PAD INS*/
        /*0x53*/ '.',  0, /*PAD DOT*/
        /*0x54*/ 0,    0,
        /*0x55*/ 0,    0,
        /*0x56*/ 0,    0,
        /*0x57*/ 0,    0, // F11
        /*0x58*/ 0,    0, // F12
        /*0x59*/ 0,    0,
        /*0x5a*/ 0,    0,
        /*0x5b*/ 0,    0,
        /*0x5c*/ 0,    0,
        /*0x5d*/ 0,    0,
        /*0x5e*/ 0,    0,
        /*0x5f*/ 0,    0,

        /*0x60*/ 0,    0,
        /*0x61*/ 0,    0,
        /*0x62*/ 0,    0,
        /*0x63*/ 0,    0,
        /*0x64*/ 0,    0,
        /*0x65*/ 0,    0,
        /*0x66*/ 0,    0,
        /*0x67*/ 0,    0,
        /*0x68*/ 0,    0,
        /*0x69*/ 0,    0,
        /*0x6a*/ 0,    0,
        /*0x6b*/ 0,    0,
        /*0x6c*/ 0,    0,
        /*0x6d*/ 0,    0,
        /*0x6e*/ 0,    0,
        /*0x6f*/ 0,    0,

        /*0x70*/ 0,    0,
        /*0x71*/ 0,    0,
        /*0x72*/ 0,    0,
        /*0x73*/ 0,    0,
        /*0x74*/ 0,    0,
        /*0x75*/ 0,    0,
        /*0x76*/ 0,    0,
        /*0x77*/ 0,    0,
        /*0x78*/ 0,    0,
        /*0x79*/ 0,    0,
        /*0x7a*/ 0,    0,
        /*0x7b*/ 0,    0,
        /*0x7c*/ 0,    0,
        /*0x7d*/ 0,    0,
        /*0x7e*/ 0,    0,
        /*0x7f*/ 0,    0,
};

struct keyboard_t {
    int buf_size;
    unsigned char *buf;
    unsigned char *head;
    unsigned char *tail;
    int count;

    bool shift_l;
} keyboard;

bool is_keyboard_init = false;

irqreturn_t do_keyboard(int irq, void *dev_id) {
    u8 x = inb(0x60);

    if (is_keyboard_init) {
        if (keyboard.head == keyboard.buf + keyboard.buf_size) {
            keyboard.head = keyboard.buf;
        }
        *keyboard.head++ = x;
        ++keyboard.count;
    }
    return IRQ_NONE;
}

struct irq_action keyboard_action = {.name = "keyboard",
                                     .handler = do_keyboard};

void keyboard_init(void) {
    keyboard.buf_size = 128;
    void *p = kmalloc(keyboard.buf_size, SLUB_NONE);
    if (!p) {
        printk("keyboard init error\n");
        keyboard.buf = NULL;
    }

    keyboard.buf = p;
    keyboard.head = p;
    keyboard.tail = p;
    keyboard.shift_l = false;
    is_keyboard_init = true;

    irq_set_handler(1, handle_simple_irq, "i8042");
    setup_irq(1, &keyboard_action);
}

unsigned char get_scancode() {
    if (!keyboard.count)
        return 0;

    if (keyboard.tail == keyboard.buf + keyboard.buf_size) {
        keyboard.tail = keyboard.buf;
    }
    unsigned char ret = *keyboard.tail;
    --keyboard.count;
    ++keyboard.tail;
    return ret;
}

int get_charcode(char *ch) {
    /* irq_disable(); */
    int retval = 0;
    if (!ch) {
        retval = -1;
        goto ret;
    }

    unsigned char x = get_scancode();

    size_t index = (x & 0x7f) * 2;
    bool state = (x & 0x80);

    if (keyboard.shift_l) {
        *ch = keycode_map_normal[index + 1];
    } else {
        *ch = keycode_map_normal[index];
    }

    if (*ch == 0x2a) {
        keyboard.shift_l = !state;
        *ch = 0;
        goto ret;
    }

    if (state) {
        *ch = 0;
        goto ret;
    }
 ret:
    /* irq_enable(); */
    return retval;
}
