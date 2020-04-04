#include <asm/irq.h>
#include <my-os/slub_alloc.h>
#include <my-os/task.h>

u64 jiffies_64 = 0;

struct cfs_rq *rq;

/*
 * Targeted preemption latency for CPU-bound tasks:
 *
 * (default: 6ms * (1 + ilog(ncpus)), units: nanoseconds)
 */
unsigned int sysctl_sched_latency = 6000000ULL;

const int sched_prio_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /*  -5 */ 3121,  2501,  1991,  1586,  1277,
    /*   0 */ 1024,  820,   655,   526,   423,
    /*   5 */ 335,   272,   215,   172,   137,
    /*  10 */ 110,   87,    70,    56,    45,
    /*  15 */ 36,    29,    23,    18,    15,
};

static inline struct task_struct *task_of(struct sched_entity *se) {
    return container_of(se, struct task_struct, se);
}

static inline struct sched_entity *sched_of(struct rb_node *node) {
    return container_of(node, struct sched_entity, run_node);
}

void context_switch(struct task_struct *prev, struct task_struct *next);

irqreturn_t do_timer(int irq, void *dev_id) {
    jiffies_64 += 1;
    return IRQ_NONE;
}

void schedule_irq_init() {
    irq_set_handler(2, handle_simple_irq, "timer");

    struct irq_action *action = kmalloc(sizeof(struct irq_action), SLUB_NONE);
    action->name = "timer";
    action->handler = do_timer;

    setup_irq(2, action);
}

void test_task1() {
    printk("test task1\n");
    context_switch(current, init_task);
    while (true)
        ;
}

void test_task2() {
    while (true)
        ;
}

#define task_top_of_stack(task) ((unsigned long)(task) + THREAD_SIZE)

extern struct task_struct *__switch_to_asm(struct task_struct *prev,
                                           struct task_struct *next);

void context_switch(struct task_struct *prev, struct task_struct *next) {
    asm volatile("movq %%rsp,%0\n\t"
                 "movq %2, %%rsp\n\t"
                 "leaq 1f(%%rip), %%rax\n\t"
                 "movq %%rax, %1\n\t"
                 "pushq %3\n\t"
                 "jmp __switch_to\n\t"
                 "1:\n\t"
                 : "=m"(prev->thread.sp), "=m"(prev->thread.ip)
                 : "m"(next->thread.sp), "m"(next->thread.ip), "D"(prev),
                   "S"(next)
                 : "memory");
}

void __switch_to(struct task_struct *prev_p,
                                struct task_struct *next_p) {
    set_current(next_p);
}

void schedule_init() {
    rq = kmalloc(sizeof(struct cfs_rq), SLUB_NONE);
    rq->tasks_timeline = RB_ROOT;
    rq->curr = &init_task->se;

    struct task_struct *task1 = kmalloc(sizeof(union thread_union), SLUB_NONE);
    task1->pid = current->pid + 1;
    list_add(&task1->tasks, &init_task->tasks);
    task1->thread.sp = task_top_of_stack(task1);
    task1->thread.ip = (phys_addr_t)test_task1;

    context_switch(current, task1);

    struct task_struct *task2 = kmalloc(sizeof(union thread_union), SLUB_NONE);
    task2->pid = task1->pid + 1;
    task2->thread.sp = task_top_of_stack(task1);
    list_add(&task2->tasks, &init_task->tasks);

    struct task_struct *task;
    list_for_each_entry(task, &init_task->tasks, tasks) {
        printk("pid %d\n", task->pid);
    }
    /* context_switch(current, task1); */
}

struct task_struct *pick_next_task() {
    return task_of(sched_of(rb_first(&rq->tasks_timeline)));
}

void _schedule() {}

void preempt_schedule_irq() { _schedule(); }
