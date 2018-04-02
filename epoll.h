#ifndef EPOLL_H
#define EPOLL_H

#include "tpool.h"

#define EPOLLEVENTS 100
#define INET_ADDR_LEN 100

void do_epoll(int listenfd, thread_pool_t *pool);

#endif
