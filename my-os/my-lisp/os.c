#include "os.h"
#include "my_lisp.tab.h"
#include "my_lisp.lex.h"
/* #include <stdarg.h> */
/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <string.h> */

int my_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintk(fmt, args);
    va_end(args);
    return ret;
}

int my_vsprintf(char *buf, const char *fmt, va_list va) {
    return vsprintf(buf, fmt, va);
}

int my_sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = my_vsprintf(buf, fmt, args);
    va_end(args);
    return ret;
}

void *my_malloc(size_t size) {
    void *ret = kmalloc(size, SLUB_NONE);
    if (!ret) {
        my_printf("malloc error %d\n", size);
        /* exit(0); */
    }
    bzero(ret, size);
    return ret;
}

void my_free(void *o) {
    if (o) {
        kfree(o);
    }
}

void *my_realloc(void *p, size_t size) {
    if (!p) {
        return my_malloc(size);
    }

    return krealloc(p, size, SLUB_NONE);
}

void *yyalloc(size_t bytes, void *yyscanner) { return my_malloc(bytes); }

void *yyrealloc(void *ptr, size_t bytes, void *yyscanner) {
    return my_realloc(ptr, bytes);
}

void yyfree(void *ptr, void *yyscanner) { my_free(ptr); }

char *my_strdup(char *s) { return strdup(s); }

double pow(double x, double y) {
    double result = 1.0;
    int i = 0;
    for (i = 0; i < y; i++) {
        result *= x;
    }
    return result;
}

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s,
             ...) {
    va_list ap;
    va_start(ap, s);

    my_printf("%d: error: ", yyget_lineno(scanner));

    my_printf(s, ap);
    my_printf("\n");
    va_end(ap);
}

void my_error(const char *msg) { my_printf(msg); }
bool IS_EOF = false;
void eof_handle(void) { IS_EOF = true; }
int errno;
