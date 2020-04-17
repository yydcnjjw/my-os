#include <asm/irq.h>
#include <my-os/slub_alloc.h>
#include <my-os/task.h>

u64 jiffies_64 = 0;

struct cfs_rq *rq;
struct cfs_rq *get_rq() {
    return rq;
}

/*
 * Targeted preemption latency for CPU-bound tasks:
 *
 * (default: 6ms * (1 + ilog(ncpus)), units: nanoseconds)
 */
/* unsigned int sysctl_sched_latency = 6000000ULL; */
unsigned int sysctl_sched_latency = 6ULL;

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

int nice(int i) { return sched_prio_to_weight[i + 20]; }

static inline u64 task_cfs_runtime(struct sched_entity *se) {
    return (sysctl_sched_latency * se->weight) / get_rq()->weight;
}

static inline u64 task_cfs_vruntime(struct sched_entity *se) {
    return (task_cfs_runtime(se) * nice(0)) / se->weight;
}

struct cfs_rq *task_cfs_rq(struct task_struct *task) {
    return task->se.cfs_rq;
}

void preempt_schedule_irq();

static void check_preempt_tick(struct cfs_rq *cfs_rq,
                               struct sched_entity *curr) {
    u64 ideal_runtime = task_cfs_runtime(curr);
    u64 delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
    if (delta_exec > ideal_runtime) {
        curr->prev_sum_exec_runtime = curr->sum_exec_runtime;
        task_of(curr)->flags = TIF_NEED_RESCHED; /* need resched */
    }
}

static void update_cfs_curr(struct cfs_rq *cfs_rq) {
    ++cfs_rq->clock_task;
    struct sched_entity *curr = cfs_rq->curr;
    ++curr->sum_exec_runtime;

    check_preempt_tick(cfs_rq, curr);
}

irqreturn_t do_timer(int irq, void *dev_id) {
    jiffies_64 += 1;

    update_cfs_curr(get_rq());
    
    return IRQ_NONE;
}

void schedule_irq_init() {
    irq_set_handler(2, handle_simple_irq, "timer");

    struct irq_action *action = kmalloc(sizeof(struct irq_action), SLUB_NONE);
    action->name = "timer";
    action->handler = do_timer;

    setup_irq(2, action);
}

extern struct task_struct *__switch_to_asm(struct task_struct *prev,
                                           struct task_struct *next);

void context_switch(struct task_struct *prev, struct task_struct *next) {
    asm volatile("pushf\n\t"
                 "pushq %%rbp\n\t"
                 "movq %%rsp,%[prev_sp]\n\t"
                 "movq %[next_sp], %%rsp\n\t"
                 "leaq 1f(%%rip), %%rax\n\t"
                 "movq %%rax, %[prev_ip]\n\t"
                 "pushq %[next_ip]\n\t"
                 "jmp __switch_to\n\t"
                 "1:\n\t"
                 "popq %%rbp\n\t"
                 "popf\n"
                 : [ prev_sp ] "=m"(prev->thread.sp),
                   [ prev_ip ] "=m"(prev->thread.ip)
                 : [ next_sp ] "m"(next->thread.sp),
                   [ next_ip ] "m"(next->thread.ip), "D"(prev), "S"(next)
                 : "memory");
}

void __switch_to(struct task_struct *prev_p, struct task_struct *next_p) {}

bool _rq_insert(struct rb_root_cached *root, struct sched_entity *data) {

    struct rb_node *rb_first = rb_first_cached(root);
    struct rb_node **new = &rb_first_cached(root), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) {
        struct sched_entity *this =
            container_of(*new, struct sched_entity, run_node);

        parent = *new;
        if (data->vruntime < this->vruntime)
            new = &((*new)->rb_left);
        else if (data->vruntime > this->vruntime)
            new = &((*new)->rb_right);
        else
            return false;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&data->run_node, parent, new);

    if (!rb_first ||
        rb_entry(rb_first, struct sched_entity, run_node)->vruntime >
            data->vruntime) {
        rb_insert_color_cached(&data->run_node, root, true);
    } else {
        rb_insert_color_cached(&data->run_node, root, false);
    }

    return true;
}

void rq_enqueue(struct sched_entity *se) {
    if (!_rq_insert(&rq->tasks_timeline, se)) {
        halt();
    }
}

void rq_dequeue(struct sched_entity *se) {
    rb_erase_cached(&se->run_node, &rq->tasks_timeline);
}

void activate_task(struct cfs_rq *rq, struct task_struct *task) {
    ++rq->nr_running;
    rq->weight += task->se.weight;

    struct sched_entity *se = &task->se;
    se->vruntime = rq->min_vruntime;

    se->vruntime += task_cfs_vruntime(se);
    --se->vruntime;

    se->cfs_rq = rq;
    se->prev_sum_exec_runtime = rq->clock_task;
    se->sum_exec_runtime = rq->clock_task;

    rq_enqueue(se);
}

void deactivate_task(struct cfs_rq *rq, struct task_struct *task) {
    --rq->nr_running;
    rq->weight -= task->se.weight;
    rq_dequeue(&task->se);
}

void rq_init() {
    struct sched_entity *init_se = &init_task->se;
    rq = kmalloc(sizeof(struct cfs_rq), SLUB_NONE);
    rq->tasks_timeline = RB_ROOT_CACHED;
    rq->curr = init_se;
    rq->min_vruntime = 0;
    rq->weight = init_se->weight;
    rq->clock_task = jiffies_64;
    rq->nr_running = 1;

    init_se->vruntime = 0;
    init_se->cfs_rq = rq;
    init_se->prev_sum_exec_runtime = rq->clock_task;
    init_se->sum_exec_runtime = rq->clock_task;
    rq_enqueue(init_se);
}

void schedule_init() { rq_init(); }

struct task_struct *pick_next_task() {
    return task_of(sched_of(rb_first_cached(&rq->tasks_timeline)));
}

void _schedule() {
    struct task_struct *prev, *next;

    current->flags = 0;

    prev = current;
    struct sched_entity *se = &current->se;
    se->vruntime += task_cfs_vruntime(se);

    rq_dequeue(se);

    rq_enqueue(se);

    next = pick_next_task();

    struct cfs_rq *cfs_rq = get_rq();
    cfs_rq->min_vruntime = next->se.vruntime;

    if (prev != next) {
        cfs_rq->curr = &next->se;        
        /* printk("prev %s next %s\n", prev->name, next->name);         */
        set_current(next);
        irq_enable();
        context_switch(prev, next);
        irq_disable();
    }
}

void preempt_schedule_irq() {
    if (current->flags == TIF_NEED_RESCHED) {
        /* irq_enable(); */
        _schedule();
        /* irq_disable(); */
    }
}
