#ifndef _REACTOR_IMPL_H_
#define _REACTOR_IMPL_H_

#include <map>
#include "events.h"
#include "event_demultiplexer.h"
#include "event_handler.h"

class ReactorImplementation
{
public:
    ReactorImplementation();
    ~ReactorImplementation();

    int regist(EventHandler *handler, event_t evt);
    int remove(EventHandler *handler);

    void event_loop(int timeout = 0);

private:
    EventDemultiplexer *demultiplexer;
    std::map<handle_t, EventHandler*> handlers;
};

#endif