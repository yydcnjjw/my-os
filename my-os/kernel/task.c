#include <my-os/task.h>

union thread_union init_thread_union = {
    .task = {.mm = &init_mm,
             .tasks = LIST_HEAD_INIT(init_thread_union.task.tasks)}};
struct task_struct *init_task = &init_thread_union.task;

struct task_struct *current_task = &init_thread_union.task;

struct task_struct *get_current(void) {
    return current_task;
}

void set_current(struct task_struct *task) { current_task = task; }
