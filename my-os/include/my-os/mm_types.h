#ifndef _MY_OS_MM_TYPES_H
#define _MY_OS_MM_TYPES_H

struct mm_struct {
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, start_stack;
};

#endif /* _MY_OS_MM_TYPES_H */
