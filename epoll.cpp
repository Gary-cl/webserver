#include "epoll.h"
#include <sys/epoll.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void print_info(int cntfd);
static void events_handle(int epollfd, struct epoll_event *events, int num, int lstfd, thread_pool_t *pool);
void *accept_request(void *arg);
static void handler_accept(int epollfd, int lstfd);
static void do_read(int epollfd, int *cntfd, thread_pool_t *pool);
static void do_write(int epollfd, int *cntfd);
static void add_event(int epollfd,int fd,int state);
static void modify_event(int epollfd,int fd,int state);
static void delete_event(int epollfd,int fd,int state);


void do_epoll(int lstfd, thread_pool_t *pool)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret = 0;

    epollfd = epoll_create(256);
    add_event(epollfd, lstfd, EPOLLIN);

    while (1)
    {
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        printf("ret = %d\n", ret);
        events_handle(epollfd, events, ret, lstfd, pool);
    }

    close(epollfd);
}

static void events_handle(int epollfd, struct epoll_event *events, int num, int lstfd, thread_pool_t *pool)
{
    for (int i = 0; i < num; i++)
    {
        int fd = events[i].data.fd;
        if ((fd == lstfd) && (events[i].events & EPOLLIN))
        {
            handler_accept(epollfd, lstfd);
        }
        else if (events[i].events & EPOLLIN)
        {
            do_read(epollfd, &(events[i].data.fd), pool);
        }
        else if (events[i].events & EPOLLOUT)
        {
            //do_write(epollfd, &(events[i].data.fd));
        }
    }
}

static void handler_accept(int epollfd, int lstfd)
{
    struct sockaddr_in client_addr;
    int cntfd = -1;
    socklen_t addr_len = sizeof(client_addr);
    if ((cntfd = accept(lstfd, (struct sockaddr *)&client_addr, &addr_len)) == -1)
    {
        perror("accept");
        exit(1);
    }
    else
    {
        add_event(epollfd, cntfd, EPOLLIN);
        print_info(cntfd);
    }
}



/* 打印socket连接信息 */
void print_info(int cntfd)
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
    char client_ip[INET_ADDR_LEN], serv_ip[INET_ADDR_LEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
    printf("server(\"%s\":%d) connect to client(\"%s\":%d)\n",
           serv_ip, ntohs(serv_addr.sin_port), client_ip, ntohs(client_addr.sin_port));
}



void *accept_request(void *arg)
{
    char buf[1024] = {0};
    const int client = *(int *)arg;

    printf("client socket = %d\n", client);
    int n = recv(client, buf, sizeof(buf), 0);
    while (n > 0)
    {
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

    return (void*)NULL;
}

static void do_read(int epollfd, int *cntfd, thread_pool_t *pool)
{
    task_node *task = (task_node *)malloc(sizeof(task_node));
    task->func = (task_handler)accept_request;
    task->arg = cntfd;
    task->next = NULL;
    // accept_request函数加入task队列
    pthread_mutex_lock(&(pool->qlock));
    add_task(pool->task_head, task);
    pthread_mutex_unlock(&(pool->qlock));
    pthread_cond_signal(&(pool->cond)); //唤醒与一个线程
    delete_event(epollfd, *cntfd, EPOLLIN);

}

static void do_write(int epollfd, int *cntfd)
{
    //暂时不用
}


static void add_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

static void delete_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
}

static void modify_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}