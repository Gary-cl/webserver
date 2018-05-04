#ifndef _REACTOR_H_
#define _REACTOR_H_

#include "event_handler.h"

#include "events.h"
#include "reactor_impl.h"

class ReactorImplementation;

class Reactor
{
public:
    Reactor();
    ~Reactor();
    static Reactor& get_instance();
    int regist(EventHandler *handler, event_t evt);
    int remove(EventHandler *handler);
    void event_loop(int timeout = 0);
 
private:
    ReactorImplementation *impl; // reactor的实现类
    static Reactor reactor;
};


#endif