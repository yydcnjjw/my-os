#include "os.h"

#include <stdarg.h>

int my_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
#ifdef MY_OS
    int ret = vprintk(fmt, args);
#else
    int ret = vprintf(fmt, args);
#endif // MY_OS

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

#ifdef MY_OS
    void *ret = kmalloc(size, SLUB_NONE);
#else
    void *ret = malloc(size);
#endif // MY_OS

    if (!ret) {
        my_printf("malloc error\n");
        /* exit(0); */
    }
    bzero(ret, size);
    return ret;
}

void my_free(void *o) {
    if (o) {
#ifdef MY_OS
        kfree(o);
#else
        free(o);
#endif // MY_OS
    }
}

void *my_realloc(void *p, size_t size) {
    if (!p) {
        return my_malloc(size);
    }

#ifdef MY_OS
    return krealloc(p, size, SLUB_NONE);
#else
    return realloc(p, size);
#endif // MY_OS
}

void *yyalloc(size_t bytes, void *yyscanner) { return my_malloc(bytes); }

void *yyrealloc(void *ptr, size_t bytes, void *yyscanner) {
    return my_realloc(ptr, bytes);
}

void yyfree(void *ptr, void *yyscanner) { my_free(ptr); }

char *my_strdup(const char *s) { return strdup(s); }

#ifdef MY_OS

int errno;

#endif // MY_OS
