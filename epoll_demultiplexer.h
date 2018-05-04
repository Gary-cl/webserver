#ifndef _EPOLL_DEMULTIPLEXER_H_
#define _EPOLL_DEMULTIPLEXER_H_

#include <vector>

class EpollDemultiplexer : public EventDemultiplexer
{
public:
    EpollDemultiplexer();
    virtual ~EpollDemultiplexer();

    virtual int wait_event(std::map<handle_t, EventHandler *> &handlers,
                           int timeout = 0);
    virtual int regist(handle_t handle, event_t evt);
    virtual int remove(handle_t handle);

private:
    int max_fd;
    int epoll_fd;
    //std::vector<struct epoll_event> evs;
};

#endif