#ifndef _SOCKET_HANDLER_H_
#define _SOCKET_HANDLER_H_

#include "thread_pool.h"

class SocketHandler : public EventHandler
{
public:
    SocketHandler(int fd);
    virtual ~SocketHandler();

    virtual handle_t get_handle() const;
    virtual void handle_read();
    virtual void handle_write(){}
    virtual void handle_error();


private:

    thread_pool_t *my_thread_pool;
    handle_t sock_fd;
};

#endif