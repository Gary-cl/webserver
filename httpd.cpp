#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <pthread.h>

#include "thread_pool.h"

#include "reactor.h"
#include "event_handler.h"
#include "listen_handler.h"
#include "events.h"
#include <fcntl.h>


#define LSTENQ 10

/* 配置socket并返回描述符 */
int socket_init(u_short *port)
{
    int lstfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lstfd == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(*port);

    if (bind(lstfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
    else
    {
        socklen_t len = sizeof(serv_addr);
        if (getsockname(lstfd, (struct sockaddr *)&serv_addr, &len) == -1)
        {
            perror("getsockname");
            exit(1);
        }
        const int INET_ADDR_LEN = 100;
        char serv_ip[INET_ADDR_LEN];
        inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
        printf("bind in socket(\"%s\":%d)\n", serv_ip, ntohs(serv_addr.sin_port));
    }

    if (listen(lstfd, LSTENQ) < 0)
    {
        perror("listen");
        exit(1);
    }

    return lstfd;
}

int main()
{
    u_short port = 0;
    int lstfd = socket_init(&port); //listen socket

    // 设置非阻塞IO
    int flags = fcntl(lstfd, F_GETFL, 0);
    fcntl(lstfd, F_SETFL, flags | O_NONBLOCK);

    // epoll监听listen socket的读事件
    Reactor &ractor = Reactor::get_instance();
    EventHandler *handler = new ListenHandler(lstfd);
    ractor.regist(handler, ReadEvent);
    while (true)
    {
        ractor.event_loop(-1);
        printf("one loop\n");
    }
    return 0;
}
