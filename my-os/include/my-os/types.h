#ifndef _MY_OS_TYPES_H
#define _MY_OS_TYPES_H
#include <stdint.h>

#define NULL ((void*)0)

typedef __signed__ char s8;
typedef unsigned char u8;

typedef __signed__ short s16;
typedef unsigned short u16;

typedef __signed__ int s32;
typedef unsigned int u32;

typedef __signed__ long long s64;
typedef unsigned long long u64;

enum {
    true = 1,
    false = 0
};

typedef _Bool bool;


#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef long ssize_t;
#endif

typedef u64 phys_addr_t;

#endif /* _MY_OS_TYPES_H */
