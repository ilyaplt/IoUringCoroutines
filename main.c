#include <stdio.h>
#include "context.h"
#include "executor.h"
#include "ring.h"
#include "utils.h"
#include "async.h"

#define SERVER_PORT 8080
#define IORING_QUEUE_SIZE 4096
#define FILE_BUFFER 2048
#define HOST_FILE "assets/test.txt"

executor_t g_executor = {0};

void main_worker(coroutine_t *context, void *args)
{
    ring_tcp_t *tcp = (ring_tcp_t*)args;

    char buffer[512];

    int result = ring_tcp_receive_await(tcp, buffer, 512);

    if (result < 0) {
        close(tcp->fd);
        free(tcp);
        return;
    }

    const char *send_text = "HTTP/1.1 200 OK\r\n"
                         "Host: localhost\r\n"
                         "Content-Length: %d\r\n"
                         "Connection: close\r\n"
                         "\r\n";

    int file_fd = open(HOST_FILE, O_RDONLY);

    if (file_fd < 0) {
        perror("open");
        exit(0);
    }

    const size_t file_sz = lseek(file_fd, 0, SEEK_END);

    result = snprintf(buffer, 512, send_text, (int) file_sz);
    ring_tcp_send_await(tcp, buffer, result);

    ring_file_t file;
    ring_file_init(&file, tcp->handle.loop, file_fd);

    char file_buffer[FILE_BUFFER];

    size_t sent = 0;

    while (sent < file_sz) {
        result = ring_file_read_await(&file, file_buffer, FILE_BUFFER, sent);

        if (result < 0) {
            perror("read");
            exit(0);
        }

        result = ring_tcp_send_await(tcp, file_buffer, result);

        if (result < 0) {
            break;
        }

        sent += result;
    }

    close(file.fd);
    close(tcp->fd);
    free(tcp);
}

void new_connection_cb(ring_listener_t *listener, int result)
{
    if (result < 0) {
        perror("accept");
        exit(0);
    }

    char address_buffer[32];
    struct sockaddr_in address = {0};
    socklen_t address_sz = sizeof(struct sockaddr_in);

    getpeername(result, (struct sockaddr*)&address, &address_sz);

    endpoint_to_string(&address, address_buffer, 32);
    printf("New connection: %s\n", address_buffer);

    ring_tcp_t *tcp = (ring_tcp_t*) malloc(sizeof(ring_tcp_t));
    ring_tcp_init(tcp, listener->handle.loop, result);

    executor_spawn(&g_executor, main_worker, tcp);
}

void ring_prepare_executor()
{
    executor_run(&g_executor);
}

int main()
{
    executor_init(&g_executor);

    ring_loop_t loop;
    ring_loop_init(&loop, IORING_QUEUE_SIZE);

    int server_fd = make_server(SERVER_PORT);
    ring_listener_t listener;
    ring_listener_init(&listener, &loop, server_fd);
    ring_listener_start(&listener, new_connection_cb);

    ring_loop_run_with_prepare(&loop, ring_prepare_executor);
    return 0;
}
