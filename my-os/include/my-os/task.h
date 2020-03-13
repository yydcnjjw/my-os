#pragma once

#include <asm/page_types.h>

#include <my-os/mm_types.h>

struct task_struct {
    volatile long state;
    struct mm_struct *mm;
};

union thread_union {
    struct task_struct task;
    unsigned long stack[THREAD_SIZE / sizeof(unsigned long)];
};

extern union thread_union init_thread_union;
