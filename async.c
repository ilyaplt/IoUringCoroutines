//
// Created by user on 3/31/24.
//

#include "async.h"

static void ring_coroutine_cb(void *data, int result)
{
    ring_handle_t *handle = (ring_handle_t*) data;

    task_t *task = (task_t*) handle->user_data;

    task->result = result;

    coroutine_enter(task->coroutine);
}

int ring_tcp_send_await(ring_tcp_t *handle, const char *buffer, size_t size)
{
    task_t task = {
            .coroutine = coroutine_current_context(),
            .result = 0
    };

    handle->handle.user_data = (void*) &task;

    ring_tcp_send(handle, buffer, size, (tcp_cb) ring_coroutine_cb);
    ring_loop_submit(handle->handle.loop);
    coroutine_yield(coroutine_current_context());

    return task.result;
}

int ring_tcp_receive_await(ring_tcp_t *handle, char *buffer, size_t size)
{
    task_t task = {
            .coroutine = coroutine_current_context(),
            .result = 0
    };

    handle->handle.user_data = (void*) &task;

    ring_tcp_receive(handle, buffer, size, (tcp_cb) ring_coroutine_cb);
    ring_loop_submit(handle->handle.loop);
    coroutine_yield(task.coroutine);

    return task.result;
}

int ring_file_read_await(ring_file_t *handle, char *buffer, size_t size, size_t offset)
{
    task_t task = {
            .coroutine = coroutine_current_context(),
            .result = 0
    };

    handle->handle.user_data = (void*) &task;

    ring_file_read(handle, buffer, size, offset, (file_cb) ring_coroutine_cb);
    ring_loop_submit(handle->handle.loop);
    coroutine_yield(task.coroutine);

    return task.result;
}

int ring_file_write_await(ring_file_t *handle, const char *buffer, size_t size, size_t offset)
{
    task_t task = {
            .coroutine = coroutine_current_context(),
            .result = 0
    };

    handle->handle.user_data = (void*) &task;

    ring_file_write(handle, buffer, size, offset, (file_cb) ring_coroutine_cb);
    ring_loop_submit(handle->handle.loop);
    coroutine_yield(task.coroutine);

    return task.result;
}