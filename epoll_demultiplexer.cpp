#include <map>
#include <vector>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include "stdio.h"
#include <stdlib.h>

#include "events.h"
#include "event_handler.h"
#include "event_demultiplexer.h"
#include "epoll_demultiplexer.h"

EpollDemultiplexer::EpollDemultiplexer() : max_fd(0)
{
    epoll_fd = epoll_create(256);
}

EpollDemultiplexer::~EpollDemultiplexer()
{
    close(epoll_fd);
}

/* 
    等待事件触发，并将事件分离处理 

    EPOLLIN:1                   可读事件              
    EPOLLOUT:4                  可写事件
    EPOLLRDHUP:8192             对端关闭连接
    EPOLLPRI:2                  紧急数据可读
    EPOLLERR:8                  描述符错误
    EPOLLHUP:16                 关联的描述符挂起
    EPOLLET:-2147483648         边缘触发模式
    EPOLLONESHOT:1073741824     关联的描述符只触发一次事件
    


    timeout：
        >0  : 定时触发
        0   : 非阻塞
        -1  : 阻塞
*/
int EpollDemultiplexer::wait_event(std::map<handle_t, EventHandler*> &handlers, int timeout)
{
    const int MAX_EVENTS = 1000; // 最多监听事件数
    struct epoll_event evs[MAX_EVENTS] = {0};
    int num = epoll_wait(epoll_fd, evs, max_fd, timeout);
    //printf("num = %d\n", num);

    if (num > 0)
    {
        for (int i = 0; i < num; ++i)
        {
            handle_t handle = evs[i].data.fd ;
            //printf("epoll_wait: fd = %d\n", handle);
            if ((EPOLLERR | EPOLLHUP) & evs[i].events)
            {
                (handlers[handle])->handle_error();
            }
            else
            {
                if ((EPOLLIN | EPOLLPRI) & evs[i].events)
                {
                    handlers[handle]->handle_read();
                }
                if (EPOLLOUT & evs[i].events)
                {
                    handlers[handle]->handle_write();
                }
            }
        }
    }
    else if (num < 0)
    {
        perror("epoll_wait");
        close(epoll_fd);
        exit(0);
    }

    return num;
}


/* 注册event到epoll */
int EpollDemultiplexer::regist(handle_t handle, event_t evt)
{
    struct epoll_event ev = {0};
    //memset(&ev, 0, sizeof(ev));
    ev.data.fd = handle;

    if ( evt & ReadEvent )
    {
        ev.events |= EPOLLIN;
    }
    if ( evt & WriteEvent )
    {
        ev.events |= EPOLLOUT;    
    }
    ev.events |= EPOLLET;

    if (0 != epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handle, &ev))
    {
        perror("epoll_ctl");
        return -errno;
    }
    else
    {
        ++max_fd;
    }
    return 0;
}

/* 从epoll移除监听事件 */
int EpollDemultiplexer::remove(handle_t handle)
{
    struct epoll_event ev;

    if (0 != epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handle, &ev))
    {
        perror("epoll_ctl");
        return -errno;
    }

    --max_fd;
    return 0;
}

