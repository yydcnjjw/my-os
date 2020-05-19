#pragma once

#include <my-os/log2.h>
#include <my-os/types.h>
#include <stdarg.h>

#define MY_OS

#define YY_NO_INPUT
#define YY_NO_UNPUT

#ifdef MY_OS
#include <my-os/string.h>
#include <my-os/slub_alloc.h>
#include <asm/errno.h>
#include "strtox.h"

static inline long long int my_strtoll(char *s, int base) {
    long long v;
    kstrtoll(s, base, &v);
    return v;
}

#define my_strtod(s) kstrtod(s)

#undef assert
#define assert(expr) ((expr) ? 0 : my_error(#expr))

void my_error(const char *);
#define YY_FATAL_ERROR(msg) my_error(msg)

typedef void FILE;
extern int errno;

#define YY_INPUT(_buf, result, max_size)

#define YYMALLOC(s) kmalloc(s, SLUB_NONE)
#define YYFREE(p) kfree(p)

#else
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define my_strtod(s) strtod(s, NULL)
#define my_strtoll(s, base) strtoll(s, NULL, base)

#endif

void *my_malloc(size_t size);
void my_free(void *);
void *my_realloc(void *p, size_t size);
char *my_strdup(const char *s);

int my_printf(const char *fmt, ...);
int my_sprintf(char *buf, const char *fmt, ...);
int my_vsprintf(char *buf, const char *fmt, va_list);
