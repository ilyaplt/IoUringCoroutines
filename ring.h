//
// Created by user on 2/22/24.
//

#ifndef RING_H
#define RING_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <liburing.h>
#include <stdlib.h>
#include <string.h>

enum {
    RING_LISTENER,
    RING_FILE,
    RING_TCP_STREAM
};

enum {
    RING_OP_READ,
    RING_OP_WRITE,
    RING_OP_CONNECT,
    RING_OP_CLOSE
};

typedef struct ring_loop {
    struct io_uring ring;
    int pending;
} ring_loop_t;

typedef struct ring_handle {
    ring_loop_t* loop;
    void* user_data;
    int type;
    int op;
} ring_handle_t;

typedef struct ring_listener {
    ring_handle_t handle;
    void(*cb)(struct ring_listener*, int);
    struct sockaddr_in address;
    socklen_t address_size;
    int fd;
} ring_listener_t;

typedef struct ring_file {
    ring_handle_t handle;
    void(*cb)(struct ring_file*, int);
    int fd;
} ring_file_t;

typedef struct ring_tcp {
    ring_handle_t handle;
    void(*cb)(struct ring_tcp*, int);
    int fd;
} ring_tcp_t;

typedef void(*listener_cb)(struct ring_listener*, int);
typedef void(*file_cb)(struct ring_file*, int);
typedef void(*tcp_cb)(struct ring_tcp*, int);

int ring_loop_init(ring_loop_t* loop, int queue_size);

#define ring_handle_set_data(x, userdata) x.handle.user_data = userdata

void ring_listener_init(ring_listener_t* handle, ring_loop_t* loop, int fd);
void ring_listener_start(ring_listener_t* handle, listener_cb cb);
void ring_listener_stop(ring_listener_t* handle);

void ring_file_init(ring_file_t* handle, ring_loop_t* loop, int fd);
void ring_file_write(ring_file_t* handle, const char* buffer, size_t size, size_t offset, file_cb cb);
void ring_file_read(ring_file_t* handle, char* buffer, size_t size, size_t offset, file_cb cb);

void ring_tcp_init(ring_tcp_t* handle, ring_loop_t* loop, int fd);
void ring_tcp_connect_v4(ring_tcp_t* handle, struct sockaddr_in* address, tcp_cb cb);
void ring_tcp_receive(ring_tcp_t* handle, char* buffer, size_t size, tcp_cb cb);
void ring_tcp_send(ring_tcp_t* handle, const char* buffer, size_t size, tcp_cb cb);
void ring_tcp_close(ring_tcp_t* handle, tcp_cb cb);

void ring_loop_submit(ring_loop_t* loop);
int ring_loop_wait(ring_loop_t* loop);
int ring_loop_wait_for_cqes(ring_loop_t* loop);
int ring_loop_run(ring_loop_t* loop);
int ring_loop_run_with_prepare(ring_loop_t* loop, void(*prepare_cb)());
void ring_loop_close(ring_loop_t* loop);

#endif