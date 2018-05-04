#ifndef _EVENT_DEMUTIPLEXER_H_
#define _EVENT_DEMUTIPLEXER_H_

#include "event_handler.h"
#include "events.h"
#include <map>


class EventDemultiplexer
{
public:
    EventDemultiplexer() {};
    virtual ~EventDemultiplexer() {};

    virtual int wait_event(std::map<handle_t , EventHandler*> &events,
                           int timeout = 0) = 0;

    virtual int regist(handle_t handle, event_t evt) = 0;
    virtual int remove(handle_t handle) = 0;
    //virtual int UnrequestEvent(handle_t handle, event_t evt) = 0;
};

#endif