#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_


typedef int handle_t;

class EventHandler
{
public:
 
    EventHandler() {}
    virtual ~EventHandler() {};

    /// 获取该handler所对应的句柄
    virtual handle_t get_handle() const = 0;
 
    virtual void handle_read() = 0;
    virtual void handle_write() = 0;
    virtual void handle_error() = 0;
};


#endif