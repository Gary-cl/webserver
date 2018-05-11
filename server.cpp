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


#define LISTENQ 10

const char * root_dir = "/home/user/Personal/code/c++/webserver/webdocs";
/* / 所指代的网页 */
const char * home_page = "index.html";


/* 配置socket并返回描述符 */
int socket_init(u_short *port)
{
    int lstfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lstfd < 0)
    {
        perror("socket");
        return -1;
    }


    int optval = 1;
    // SO_REUSEADDR:立即关闭socket，不用等待TIME_WAIT
    if (setsockopt(lstfd, SOL_SOCKET, SO_REUSEADDR,
        (const void *)&optval, sizeof(int)) < 0) /* 设置端口复用 */
    {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(*port);
    if (bind(lstfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        return -1;
    }

    // 打印连接信息
    socklen_t len = sizeof(serv_addr);
    if (getsockname(lstfd, (struct sockaddr *)&serv_addr, &len) < 0)
    {
        perror("getsockname");
        return -1;
    }
    const int INET_ADDR_LEN = 100;
    char serv_ip[INET_ADDR_LEN];
    inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
    printf("bind in socket(\"%s\":%d)\n", serv_ip, ntohs(serv_addr.sin_port));


    if (listen(lstfd, LISTENQ) < 0)
    {
        perror("listen");
        return -1;
    }
    return lstfd;
}

int setnonblocking(int fd) /* 将文件描述符设置为非阻塞 */
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

int main()
{
    u_short port = 8080;
    int lstfd = socket_init(&port); //listen socket

    // 设置非阻塞IO
    setnonblocking(lstfd);

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
