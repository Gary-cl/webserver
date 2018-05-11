#ifndef _LISTEN_HANDLER_H_
#define _LISTEN_HANDLER_H_

//#include "event_handler.h"
#include <pthread.h>

class ListenHandler : public EventHandler
{
public:

    ListenHandler(int fd);
    virtual ~ListenHandler();

    virtual handle_t get_handle() const { return listen_fd; }
    virtual void handle_read();
    virtual void handle_write() {}
    virtual void handle_error(); 

private:
    handle_t listen_fd;
};

#endif