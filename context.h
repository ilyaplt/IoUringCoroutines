//
// Created by user on 3/30/24.
//

#ifndef UCONTEXT_TEST2_CONTEXT_H
#define UCONTEXT_TEST2_CONTEXT_H

#include <ucontext.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

typedef struct coroutine_s {
    ucontext_t context;
} coroutine_t;

typedef void(*coroutine_entry_point)(coroutine_t*, void*);

void coroutine_spawn(coroutine_t *coroutine, coroutine_entry_point entry, void *user_data);
void coroutine_enter(coroutine_t *coroutine);
void coroutine_yield(coroutine_t *coroutine);
void coroutine_free(coroutine_t *coroutine);
coroutine_t* coroutine_current_context();

#endif //UCONTEXT_TEST2_CONTEXT_H
