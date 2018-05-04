#ifndef _SOCKET_HANDLER_H_
#define _SOCKET_HANDLER_H_

#include "thread_pool.h"

class SocketHandler : public EventHandler
{
public:
    SocketHandler(int fd);
    virtual ~SocketHandler();

    virtual handle_t get_handle() const;
    virtual void handle_read(int *cntfd);
    virtual void handle_write(int *cntfd);
    virtual void handle_error();


private:
    void my_read(int *cntfd);
    void my_write(int *cntfd);

    thread_pool_t *my_thread_pool;
    handle_t sock_fd;
};

#endif