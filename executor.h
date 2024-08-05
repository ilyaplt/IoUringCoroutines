//
// Created by user on 3/31/24.
//

#ifndef UCONTEXT_TEST2_EXECUTOR_H
#define UCONTEXT_TEST2_EXECUTOR_H

#include <sys/queue.h>
#include "context.h"

typedef struct executor_task_s {
    TAILQ_ENTRY(executor_task_s) entries;
    coroutine_t context;
    coroutine_entry_point entry_point;
    void *args;
    char finished;
} executor_task_t;

typedef struct executor_s {
    TAILQ_HEAD(executor_pending_head, executor_task_s) pending;
    TAILQ_HEAD(executor_active_head, executor_task_s) active;
} executor_t;

void executor_init(executor_t *executor);
void executor_spawn(executor_t *executor, coroutine_entry_point entry, void *args);
void executor_run(executor_t *executor);

#endif //UCONTEXT_TEST2_EXECUTOR_H
