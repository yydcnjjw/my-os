#pragma once

#include <asm/page_types.h>

#include <my-os/mm_types.h>
#include <my-os/rbtree.h>

struct cfs_rq;

struct sched_entity {
    struct rb_node run_node;
    u64 vruntime;
    u64 weight;
    u64 sum_exec_runtime;
    u64 prev_sum_exec_runtime;
    struct cfs_rq *cfs_rq;
};

struct cfs_rq {
    struct rb_root tasks_timeline;
    struct sched_entity *curr;
    u64 weight;
    u32 nr_running;
    u64 min_vruntime;
    u64 clock_task;
};

struct thread_struct {
    unsigned long sp;
    unsigned long ip;

    unsigned short es;
    unsigned short ds;
    unsigned long fsbase;
    unsigned long gsbase;

    unsigned long cr2;
    unsigned long trap_nr;
    unsigned long error_code;
};

struct task_struct {
    volatile long state;
    struct sched_entity se;
    struct mm_struct *mm;
    long pid;
    struct list_head tasks;
    struct thread_struct thread;
};

union thread_union {
    struct task_struct task;
    unsigned long stack[THREAD_SIZE / sizeof(unsigned long)];
};

extern union thread_union init_thread_union;
struct task_struct *init_task;

extern struct task_struct *get_current(void);
#define current get_current()
void set_current(struct task_struct *task);

void schedule_init();
void schedule_irq_init();
