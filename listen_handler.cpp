#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "stdio.h"

#include <map>

#include "event_handler.h"
#include "socket_handler.h"
#include "reactor.h"
#include "listen_handler.h"

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

ListenHandler::ListenHandler(int fd) : listen_fd(fd)
{

}

ListenHandler::~ListenHandler()
{
    close(listen_fd);
}


/* 打印socket连接信息 */
void pinfo(int cntfd)
{
    struct sockaddr_in client_addr, serv_addr;
    socklen_t client_len = sizeof(client_addr), serv_len = sizeof(serv_addr);
    if (getsockname(cntfd, (struct sockaddr *)&serv_addr, &serv_len) == -1)
    {
        perror("getsockname");
        exit(1);
    }
    if (getpeername(cntfd, (struct sockaddr *)&client_addr, &client_len) == -1)
    {
        perror("getpeername");
        exit(1);
    }

    // ip地址长度
    const int INET_ADDR_LEN = 100;

    char client_ip[INET_ADDR_LEN], serv_ip[INET_ADDR_LEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
    printf("server(\"%s\":%d) connect to client(\"%s\":%d)\n",
           serv_ip, ntohs(serv_addr.sin_port), client_ip, ntohs(client_addr.sin_port));
}


void ListenHandler::handle_read()
{
    int fd = -1;
    
    // 非阻塞调用 accept
    do
    {
        if ((fd = accept( listen_fd, NULL, NULL )) < 0)
        {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN))   // 数据读取完毕
            {
                break;
            }
            perror("accept");
            exit(1);
        }
        printf("client socket = %d\n", fd);
        pinfo(fd);
        

        // 设置非阻塞IO
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        // 为已连接的socket注册可读事件
        EventHandler *h = new SocketHandler(fd);
        Reactor &r = Reactor::get_instance();
        r.regist(h, ReadEvent);
    } while (true);

}

void ListenHandler::handle_error()
{
    
}
