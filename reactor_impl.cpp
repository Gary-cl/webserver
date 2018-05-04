#include "reactor_impl.h"
#include "epoll_demultiplexer.h"
#include <stdio.h>

ReactorImplementation::ReactorImplementation()
{
    demultiplexer = new EpollDemultiplexer();
}

ReactorImplementation::~ReactorImplementation()
{
    for (std::map<handle_t, EventHandler *>::iterator it = handlers.begin(); it != handlers.end(); ++it)
    {
        delete it->second;
    }

    if (demultiplexer)
        delete demultiplexer;    
}



/*  将handler加入handlers
    将evt注册到epoll    */
int ReactorImplementation::regist(EventHandler *handler, event_t evt)
{
    handle_t handle = handler->get_handle();

    if (handlers.end() == handlers.find(handle))
    {
        handlers.insert(std::make_pair<handle_t, EventHandler*>(handle, handler));
    }

    return demultiplexer->regist(handle, evt);
}


/*  从handlers移除handler
    从epoll注销对应描述符的事件
    销毁handler             */
int ReactorImplementation::remove(EventHandler *handler)
{
    handle_t handle = handler->get_handle();
    demultiplexer->remove(handle);

    std::map<handle_t, EventHandler *>::iterator it = handlers.find(handle);
    
    delete it->second;
    handlers.erase(handle);
}

void ReactorImplementation::event_loop(int timeout)
{
    demultiplexer->wait_event(handlers, timeout);
}

