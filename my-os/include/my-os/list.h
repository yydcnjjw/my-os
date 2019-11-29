#ifndef _MY_OS_LIST_H
#define _MY_OS_LIST_H

#include <my-os/types.h>
#include <my-os/kernel.h>

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name)                                                   \
    { &(name), &(name) }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *new, struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

#define list_for_each(pos, head)                                               \
    for (pos = (head)->next; pos != (head); pos = (pos)->next)

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_next_entry(pos, member)                                           \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member)                                 \
    for (pos = list_next_entry(head, member); &pos->member != &(head)->member; \
         pos = list_next_entry(pos, member))

static inline int list_len(struct list_head *head) {
    struct list_head *p;
    int i = 0;
    list_for_each(p, head) { i++; }
    return i;
}

#endif /* _MY_OS_LIST_H */
