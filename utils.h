//
// Created by user on 3/30/24.
//

#ifndef UCONTEXT_TEST2_UTILS_H
#define UCONTEXT_TEST2_UTILS_H

#include <stdint.h>

int make_server(uint16_t port);
int endpoint_to_string(struct sockaddr_in *address, char *buffer, int size);

#endif //UCONTEXT_TEST2_UTILS_H
