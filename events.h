#ifndef _EVENTS_H_
#define _EVENTS_H_


typedef unsigned int event_t;

enum EventMask
{
    ReadEvent   = 0x01,
    WriteEvent  = 0x02,
    ErrorEvent  = 0x04,
    EventMask   = 0xff
};


#endif