#include "my_lisp.h"
#include <kernel/keyboard.h>

ssize_t get_expr_str(char *lineptr, size_t n) {
    char ch;
    if (!lineptr) {
        return -2;
    }
    char *ptr = lineptr;
    for (;;) {
        int ret = get_charcode(&ch);
        if (ret) {
            printk("ch %p\n", &ch);
            return -1;
        }

        if (ptr == lineptr + n - 1) {
            return n - 1;
        }

        switch (ch) {
        case '\n':
            printk("\n");
            *ptr++ = ch;
            return ptr - lineptr;
        case '\b':
            if (ptr != lineptr) {
                vga_backspace();
                *ptr-- = 0;
            }
            break;
        case 0:
            break;
        default:
            *ptr++ = ch;
            printk("%c", ch);
            break;
        }
    }
}

int my_lisp_boot(void) {

    struct lisp_ctx_opt opt = {};
    struct lisp_ctx *ctx = make_lisp_ctx(opt);

    char *buf = my_malloc(PTE_SIZE);
    while (true) {
        bzero(buf, PTE_SIZE);
        printk("> ");
        int code = get_expr_str(buf, PTE_SIZE);
        if (code == -1) {
            printk("getline error\n");
            break;
        } else if (code == -2) {
            printk("getline error ptr null\n");
            break;
        }
        object *o = eval_from_str(ctx, buf);
        object_print(o, ctx->global_env);
        my_printf("\n");
    }
    my_free(buf);
    return 0;
}
