#include "reactor.h"

Reactor Reactor::reactor;

Reactor& Reactor::get_instance()
{
    return reactor;
}

Reactor::Reactor()
{
    impl = new ReactorImplementation();
}

Reactor::~Reactor()
{
    if (impl)
        delete impl;
}


int Reactor::regist(EventHandler *handler, event_t evt)
{
    return impl->regist(handler, evt);
}

int Reactor::remove(EventHandler *handler)
{
    return impl->remove(handler);
}

void Reactor::event_loop(int timeout)
{
    impl->event_loop(timeout);
}

