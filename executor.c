//
// Created by user on 3/31/24.
//

#include "executor.h"

static void executor_task_created(coroutine_t *context, void *args)
{
    executor_task_t *task = (executor_task_t*) args;

    task->entry_point(context, task->args);

    task->finished = 1;
}

void executor_init(executor_t *executor)
{
    memset(executor, 0, sizeof(executor_t));
    TAILQ_INIT(&executor->pending);
    TAILQ_INIT(&executor->active);
}

void executor_spawn(executor_t *executor, coroutine_entry_point entry, void *args)
{
    executor_task_t *task = (executor_task_t*) malloc(sizeof(executor_task_t));

    memset(task, 0, sizeof(executor_task_t));

    task->entry_point = entry;
    task->args = args;

    coroutine_spawn(&task->context, executor_task_created, task);

    TAILQ_INSERT_TAIL(&executor->pending, task, entries);
}

static void executor_drain(executor_t *executor)
{
    executor_task_t *task = executor->active.tqh_first;

    while (task != NULL) {
        executor_task_t *next = task->entries.tqe_next;

        if (task->finished == 1) {
            TAILQ_REMOVE(&executor->active, task, entries);
            coroutine_free(&task->context);
            free(task);
        }

        task = next;
    }
}

void executor_run(executor_t *executor)
{
    executor_drain(executor);

    executor_task_t *task = executor->pending.tqh_first;

    while (task != NULL) {
        coroutine_enter(&task->context);

        TAILQ_REMOVE(&executor->pending, executor->pending.tqh_first, entries);
        TAILQ_INSERT_TAIL(&executor->active, task, entries);

        task = executor->pending.tqh_first;
    }
}