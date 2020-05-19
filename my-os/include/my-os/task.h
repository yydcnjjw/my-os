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
    struct rb_root_cached tasks_timeline;
    struct sched_entity *curr;
    u64 weight;
    u32 nr_running;
    u64 min_vruntime;
    u64 clock_task;
};

struct cfs_rq *get_rq();

struct thread_struct {
    unsigned long sp;
    unsigned long ip;
};

struct task_struct {
    volatile long state;
    struct sched_entity se;
    struct mm_struct *mm;
    u32 pid;
    u32 flags;
    const char *name;
    struct list_head tasks;
    struct thread_struct thread;
};

#define TIF_NEED_RESCHED 3 /* rescheduling necessary */

union thread_union {
    struct task_struct task;
    unsigned long stack[THREAD_SIZE / sizeof(unsigned long)];
};

extern union thread_union init_thread_union;
extern struct task_struct *init_task;

extern struct task_struct *get_current(void);
#define current get_current()
void set_current(struct task_struct *task);
#define task_top_of_stack(task) ((unsigned long)(task) + THREAD_SIZE)

typedef void (worker_routine)();

struct task_struct *create_task(const char *name, worker_routine routine);

// sched
static inline struct task_struct *task_of(struct sched_entity *se) {
    return container_of(se, struct task_struct, se);
}

static inline struct sched_entity *sched_of(struct rb_node *node) {
    return container_of(node, struct sched_entity, run_node);
}

void schedule_init();
void schedule_irq_init();
void rq_enqueue(struct sched_entity *se);
void rq_dequeue(struct sched_entity *se);
void activate_task(struct cfs_rq *rq, struct task_struct *task);
void deactivate_task(struct cfs_rq *rq, struct task_struct *task);
int nice(int i);
void context_switch(struct task_struct *prev, struct task_struct *next);
extern u64 jiffies_64;
