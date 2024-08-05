//
// Created by user on 2/22/24.
//

#include "ring.h"
#include <stdio.h>

#define GET_SQE(x) struct io_uring_sqe* sqe = io_uring_get_sqe(&handle->handle.loop->ring)
#define PREPARE_SQE(handle) { sqe->user_data = (__u64)handle;\
sqe->fd = handle->fd;\
}

#define RING_CQE_BUFFER_SIZE 512

static inline void ring_loop_add_cnt(ring_loop_t* loop) {
    ++(loop->pending);
}

int ring_loop_init(ring_loop_t* loop, int queue_size) {
    memset(loop, 0, sizeof(ring_loop_t));

    struct io_uring_params params = {0};

    return io_uring_queue_init_params(queue_size, &loop->ring, &params);
}

void ring_listener_init(ring_listener_t* handle, ring_loop_t* loop, int fd) {
    memset(handle, 0, sizeof(ring_listener_t));

    handle->handle.loop = loop;
    handle->handle.type = RING_LISTENER;

    handle->fd = fd;
}

void ring_tcp_init(ring_tcp_t* handle, ring_loop_t* loop, int fd) {
    memset(handle, 0, sizeof(ring_tcp_t));

    handle->fd = fd;

    handle->handle.loop = loop;
    handle->handle.type = RING_TCP_STREAM;
}

static void ring_listener_prepare_sqe(ring_listener_t* handle) {
    GET_SQE(handle);

    PREPARE_SQE(handle);

    handle->address_size = 0;
    memset(&handle->address, 0, sizeof(struct sockaddr_in));

    io_uring_prep_accept(sqe, handle->fd, (struct sockaddr*)&handle->address, &handle->address_size, 0);
    io_uring_sqe_set_data(sqe, handle);

    ring_loop_add_cnt(handle->handle.loop);
}

void ring_listener_start(ring_listener_t* handle, listener_cb cb) {
    handle->cb = cb;

    ring_listener_prepare_sqe(handle);
}

void ring_listener_stop(ring_listener_t* handle) {
    GET_SQE(handle);

    sqe->user_data = (__u64)handle;

    io_uring_prep_cancel(sqe, handle, 0);
}

void ring_loop_submit(ring_loop_t* loop) {
    io_uring_submit(&loop->ring);
}

static void ring_loop_dispatch_listener(ring_listener_t* listener, struct io_uring_cqe* cqe) {
    int client_fd = cqe->res;

    if (listener->cb) {
        listener->cb(listener, client_fd);
    }

    ring_listener_prepare_sqe(listener);
}

static void ring_loop_dispatch_file(ring_file_t* file, struct io_uring_cqe* cqe) {
    if (file->cb) {
        file->cb(file, cqe->res);
    }
}

static void ring_loop_dispatch_tcp(ring_tcp_t* tcp, struct io_uring_cqe* cqe) {
    if (tcp->cb) {
        tcp->cb(tcp, cqe->res);
    }
}

static void ring_loop_dispatch(ring_loop_t* loop, struct io_uring_cqe* cqe) {
    if (!cqe->user_data) return; // todo

    ring_handle_t* handle = (ring_handle_t*)cqe->user_data;

    switch (handle->type) {
        case RING_LISTENER:
            if (cqe->res <= 0) {
                io_uring_cqe_seen(&loop->ring, cqe);
                break;
            }

            ring_loop_dispatch_listener((ring_listener_t*)handle, cqe);
            break;
        case RING_FILE:
            ring_loop_dispatch_file((ring_file_t*)handle, cqe);
            break;
        case RING_TCP_STREAM:
            ring_loop_dispatch_tcp((ring_tcp_t*)handle, cqe);
            break;
    }

    io_uring_cqe_seen(&loop->ring, cqe);
    --loop->pending;
}

void ring_file_init(ring_file_t* handle, ring_loop_t* loop, int fd) {
    memset(handle, 0, sizeof(ring_file_t));

    handle->fd = fd;

    handle->handle.loop = loop;
    handle->handle.type = RING_FILE;
}

static void ring_file_prepare_sqe(ring_file_t* handle, const char* buffer, size_t size, size_t offset, int op) {
    GET_SQE(handle);

    PREPARE_SQE(handle);

    io_uring_sqe_set_data(sqe, handle);
    ring_loop_add_cnt(handle->handle.loop);

    switch (op) {
        case RING_OP_WRITE:
        {
            io_uring_prep_write(sqe, handle->fd, buffer, size, offset);
            handle->handle.op = RING_OP_WRITE;
        }
            break;
        case RING_OP_READ:
        {
            io_uring_prep_read(sqe, handle->fd, (void*)buffer, size, offset);
            handle->handle.op = RING_OP_READ;
        }
            break;
    }
}

static void ring_tcp_prepare_sqe(ring_tcp_t* handle, char* buffer, size_t size, int op) {
    GET_SQE(handle);

    PREPARE_SQE(handle);

    io_uring_sqe_set_data(sqe, handle);
    ring_loop_add_cnt(handle->handle.loop);

    switch (op) {
        case RING_OP_READ:
        {
            io_uring_prep_recv(sqe, handle->fd, buffer, size, 0);
            handle->handle.op = RING_OP_READ;
        }
            break;
        case RING_OP_WRITE:
        {
            io_uring_prep_send(sqe, handle->fd, buffer, size, 0);
            handle->handle.op = RING_OP_WRITE;
        }
            break;
        case RING_OP_CONNECT:
        {
            io_uring_prep_connect(sqe, handle->fd, (struct sockaddr*)buffer, size);
            handle->handle.op = RING_OP_CONNECT;
        }
            break;
        case RING_OP_CLOSE:
        {
            io_uring_prep_close(sqe, handle->fd);
            handle->handle.op = RING_OP_CLOSE;
        }
            break;
    }
}

void ring_tcp_receive(ring_tcp_t* handle, char* buffer, size_t size, tcp_cb cb) {
    handle->cb = cb;
    return ring_tcp_prepare_sqe(handle, buffer, size, RING_OP_READ);
}

void ring_tcp_send(ring_tcp_t* handle, const char* buffer, size_t size, tcp_cb cb) {
    handle->cb = cb;
    return ring_tcp_prepare_sqe(handle, (char*)buffer, size, RING_OP_WRITE);
}

void ring_tcp_connect_v4(ring_tcp_t* handle, struct sockaddr_in* address, tcp_cb cb) {
    handle->cb = cb;
    return ring_tcp_prepare_sqe(handle, (char*)address, sizeof(struct sockaddr_in), RING_OP_CONNECT);
}

void ring_file_write(ring_file_t* handle, const char* buffer, size_t size, size_t offset, file_cb cb) {
    handle->cb = cb;
    return ring_file_prepare_sqe(handle, buffer, size, offset, RING_OP_WRITE);
}

void ring_file_read(ring_file_t* handle, char* buffer, size_t size, size_t offset, file_cb cb) {
    handle->cb = cb;
    return ring_file_prepare_sqe(handle, buffer, size, offset, RING_OP_READ);
}

int ring_loop_wait(ring_loop_t* loop) {
    struct io_uring_cqe* cqe = NULL;

    int res = io_uring_wait_cqe(&loop->ring, &cqe);

    if (res < 0) {
        return res;
    }

    ring_loop_dispatch(loop, cqe);

    return res;
}

int ring_loop_wait_for_cqes(ring_loop_t* loop) {
    struct io_uring_cqe* cqe = NULL;

    int res = io_uring_wait_cqe(&loop->ring, &cqe);

    if (res < 0) {
        return res;
    }

    struct io_uring_cqe* cqes[RING_CQE_BUFFER_SIZE];

    res = io_uring_peek_batch_cqe(&loop->ring, cqes, RING_CQE_BUFFER_SIZE);

    if (res == 0) {
        return res;
    }

    for (int i = 0; i < res; ++i) {
        ring_loop_dispatch(loop, cqes[i]);
    }

    return res;
}

void ring_tcp_close(ring_tcp_t* handle, tcp_cb cb) {
    handle->cb = cb;
    return ring_tcp_prepare_sqe(handle, NULL, 0, RING_OP_CLOSE);
}

int ring_loop_run(ring_loop_t* loop) {
    while (loop->pending > 0) {
        int res = ring_loop_wait_for_cqes(loop);

        if (res < 0) {
            return res;
        }

        ring_loop_submit(loop);
    }

    return 0;
}

int ring_loop_run_with_prepare(ring_loop_t* loop, void(*prepare_cb)()) {
    prepare_cb();

    ring_loop_submit(loop);

    while (loop->pending > 0) {
        prepare_cb();
        int res = ring_loop_wait_for_cqes(loop);

        if (res < 0) {
            return res;
        }

        ring_loop_submit(loop);
    }

    return 0;
}

void ring_loop_close(ring_loop_t* loop) {
    io_uring_queue_exit(&loop->ring);
}
