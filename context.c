#include "context.h"
#include <stdio.h>

#define DEFAULT_STACK_SIZE (1024 * 256)

static __thread ucontext_t main_context = {0};
static __thread coroutine_t *current_context = NULL;

static void* alloc_stack(size_t size)
{
    void *ptr = mmap(NULL,
                     size,
                     PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE,
                     0,
                     0);

    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(0);
    }

    return ptr;
}

void coroutine_spawn(coroutine_t *coroutine, coroutine_entry_point entry, void *user_data)
{
    ucontext_t *context = &coroutine->context;
    memset(context, 0, sizeof(ucontext_t));

    void *stack_ptr = alloc_stack(DEFAULT_STACK_SIZE);

    context->uc_stack.ss_sp = stack_ptr;
    context->uc_stack.ss_size = DEFAULT_STACK_SIZE;
    context->uc_link = &main_context;

    getcontext(context);
    makecontext(context, (void(*)(void)) entry, 2, coroutine, user_data);
}

void coroutine_enter(coroutine_t *coroutine)
{
    current_context = coroutine;
    swapcontext(&main_context, &coroutine->context);
}

void coroutine_yield(coroutine_t *coroutine)
{
    current_context = NULL;

    swapcontext(&coroutine->context, coroutine->context.uc_link);
}

coroutine_t* coroutine_current_context()
{
    return current_context;
}

void coroutine_free(coroutine_t *coroutine)
{
    munmap(coroutine->context.uc_stack.ss_sp, coroutine->context.uc_stack.ss_size);
}