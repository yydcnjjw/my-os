#ifndef X86_KERNEL_STRING_H
#define X86_KERNEL_STRING_H

#include <my-os/types.h>

size_t strlen(const char *);

void *memset(void *s, int c, size_t n);

void *bzero(void *s, size_t n);

int strcmp(const char *cs, const char *ct);

void *memmove(void *dest, const void *src, size_t count);

void *memcpy(void *dest, const void *src, size_t count);

char *strcat(char *dest, const char *src);

char *strchr(const char *s, int c);

int memcmp(const void *cs, const void *ct, size_t count);

char *strdup(char *s);

#endif /* X86_KERNEL_STRING_H */
