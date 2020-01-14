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
