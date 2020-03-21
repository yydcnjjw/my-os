#include "my_lisp.h"
#include "my_lisp.tab.h"
#include "my_lisp.lex.h"

#include <kernel/keyboard.h>

ssize_t get_expr_str(char *lineptr, size_t n) {
    char ch;
    if (!lineptr) {
        return -1;
    }
    char *ptr = lineptr;
    for (;;) {
        int ret = get_charcode(&ch);
        if (ret) {
            return -1;
        }

        if (ptr == lineptr + n - 1) {
            return n - 1;
        }

        switch (ch) {
        case '\n':
            printk("\n");
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

void init_parse_data(parse_data *data) {
    if (!data) {
        my_printf("init parse data failure\n");
    }

    data->ast = NULL;
    data->symtab = my_malloc(NHASH * sizeof(symbol *));
}

int my_lisp_boot(void) {
    parse_data data;
    init_parse_data(&data);
    yyscan_t scanner;

    env *env = new_env();
    env_add_primitives(env, &data);

    if (yylex_init_extra(&data, &scanner)) {
        my_printf("init alloc failed");
        return 1;
    }

    char *buf = my_malloc(PTE_SIZE);
    while (true) {
        bzero(buf, PTE_SIZE);
        if (get_expr_str(buf, PTE_SIZE) == -1) {
            printk("getline error\n");
            break;
        }
        YY_BUFFER_STATE my_string_buffer = yy_scan_string(buf, scanner);
        yy_switch_to_buffer(my_string_buffer, scanner);

        int ret = yyparse(scanner, &data);
        yy_delete_buffer(my_string_buffer, scanner);

        if (!ret) {
            my_printf("eval: ");
            object_print(ref(data.ast), env);
            my_printf("\n");

            object *value = eval(data.ast, env, &data);
            object_print(value, env);
            my_printf("\n");
            data.ast = NULL;
        } else {
            break;
        }
    }
    my_free(buf);

    yylex_destroy(scanner);
    free_env(env);
    free_lisp(&data);
    return 0;
}
