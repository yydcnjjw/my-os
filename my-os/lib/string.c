#include <my-os/slub_alloc.h>
#include <my-os/string.h>

size_t strlen(const char *s) {
    const char *sc;
    for (sc = s; *sc != '\0'; ++sc)
        /* nothing */;
    return sc - s;
}

void *memset(void *s, int c, size_t count) {
    char *xs = s;

    while (count--)
        *xs++ = c;
    return s;
}

void *bzero(void *s, size_t n) { return memset(s, 0, n); }

int strcmp(const char *cs, const char *ct) {
    unsigned char c1, c2;

    while (1) {
        c1 = *cs++;
        c2 = *ct++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t count) {
    char *tmp;
    const char *s;

    if (dest <= src) {
        tmp = dest;
        s = src;
        while (count--)
            *tmp++ = *s++;
    } else {
        tmp = dest;
        tmp += count;
        s = src;
        s += count;
        while (count--)
            *--tmp = *--s;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    char *tmp = dest;
    const char *s = src;

    while (count--)
        *tmp++ = *s++;
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *tmp = dest;

    while (*dest)
        dest++;
    while ((*dest++ = *src++) != '\0')
        ;
    return tmp;
}

char *strchr(const char *s, int c) {
    for (; *s != (char)c; ++s)
        if (*s == '\0')
            return NULL;
    return (char *)s;
}

int memcmp(const void *cs, const void *ct, size_t count) {
    const unsigned char *su1, *su2;
    int res = 0;

    for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
    return res;
}

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new = kmalloc(len, SLUB_NONE);
    memcpy(new, s, len );
    return new;
}
