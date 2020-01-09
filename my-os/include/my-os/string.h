#ifndef X86_KERNEL_STRING_H
#define X86_KERNEL_STRING_H

#include <my-os/types.h>

size_t strlen(const char *);
void *memset(void *s, int c, size_t n);
void *bzero(void *s, size_t n);
int strcmp(const char *cs, const char *ct);
void *memmove(void *dest, const void *src, size_t count);

#endif /* X86_KERNEL_STRING_H */