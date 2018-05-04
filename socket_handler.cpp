#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <map>

#include "event_handler.h"
#include "reactor.h"
#include "socket_handler.h"
#include "thread_pool.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <errno.h>


SocketHandler::SocketHandler(int fd) : sock_fd(fd)
{
    my_thread_pool = thread_pool_init(20);
}

SocketHandler::~SocketHandler()
{
}

handle_t SocketHandler::get_handle() const
{
    return sock_fd;
}

/*  读IO 
    接收cliient发来的数据n，并返回n个‘x’字符 */
static void *real_read(void *arg)
{
    char buf[1024] = {0};
    const int client = *(int *)arg;

    printf("client socket = %d\n", client);
    int n = recv(client, buf, sizeof(buf), 0);
    while (n > 0)
    {
        printf("recv: %s\n", buf);
        int num = atoi(buf);
        memset(buf, 0, sizeof(buf));
        for (int i = 0; i < num; i++)
        {
            buf[i] = 'x';
        }
        if (send(client, buf, strlen(buf), 0) < 0)
        {
            close(client);
            perror("send");
            exit(1);
        }
        memset(buf, 0, sizeof(buf));
        n = recv(client, buf, sizeof(buf), 0);
    }

    close(client);
    printf("close socket = %d\n", client);
    return (void*)NULL;
}


void SocketHandler::handle_read(int *cntfd)
{
    task_node *task = (task_node *)malloc(sizeof(task_node));
    task->callback = (task_handler)real_read;
    task->arg = cntfd;
    task->next = NULL;
    
    // real_read加入task链表
    pthread_mutex_lock(&(my_thread_pool->qlock));
    add_task(my_thread_pool->task_head, task);
    pthread_mutex_unlock(&(my_thread_pool->qlock));

    
    /*
        为了保证close前调用epoll_ctl注销事件
        handle_error中epoll_ctl注销了该描述符的事件
        real_read函数中close描述符
    */
    handle_error();
    pthread_cond_signal(&(my_thread_pool->cond)); //唤醒与一个线程    
}

void SocketHandler::handle_write(int *cntfd)
{
    /* 暂未实现 */
}

void SocketHandler::handle_error()
{
    Reactor& r = Reactor::get_instance();
    r.remove( this );
}

