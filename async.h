//
// Created by user on 3/31/24.
//

#ifndef UCONTEXT_TEST2_ASYNC_H
#define UCONTEXT_TEST2_ASYNC_H

#include "context.h"
#include "ring.h"

typedef struct task_s {
    coroutine_t *coroutine;
    int result;
} task_t;

int ring_tcp_send_await(ring_tcp_t *handle, const char *buffer, size_t size);
int ring_tcp_receive_await(ring_tcp_t *handle, char *buffer, size_t size);

int ring_file_read_await(ring_file_t *handle, char *buffer, size_t size, size_t offset);
int ring_file_write_await(ring_file_t *handle, const char *buffer, size_t size, size_t offset);

#endif //UCONTEXT_TEST2_ASYNC_H
