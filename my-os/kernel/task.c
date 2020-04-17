#include <my-os/slub_alloc.h>
#include <my-os/task.h>
#include <asm/irq.h>

union thread_union init_thread_union = {
    .task = {.mm = &init_mm,
             .tasks = LIST_HEAD_INIT(init_thread_union.task.tasks),
             .se = {.weight = 1024},
             .name = "init"}};

struct task_struct *init_task = &init_thread_union.task;

struct task_struct *current_task = &init_thread_union.task;

struct task_struct *get_current(void) {
    return current_task;
}

void set_current(struct task_struct *task) { current_task = task; }

struct task_struct *create_task(const char *name, worker_routine routine) {
    /* irq_disable();    */
    struct task_struct *task = kmalloc(sizeof(union thread_union), SLUB_NONE);
    task->pid = current->pid + 1;
    task->name = name;
    task->thread.sp = task_top_of_stack(task);
    task->thread.ip = (phys_addr_t)routine;

    list_add(&task->tasks, &init_task->tasks);
    printk("task stack %p\n", task);
    printk("task stack %p\n", task->thread.sp);
    task->se.weight = nice(0);
    activate_task(get_rq(), task);
    /* irq_enable(); */
    return task;
}
