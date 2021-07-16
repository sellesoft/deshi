#pragma once
#ifndef DESHI_EVENT_H
#define DESHI_EVENT_H

#include "../defines.h"

#include <set>

struct Receiver;

enum EventBits : u32 {
	Event_NONE = 0,
	Event_DoorToggle,
    Event_LightToggle
}; typedef u32 Event;

global_ const char* EventStrings[] = {
    "NONE", "DoorToggle", "LightToggle"
};

//object stored on a component that wants to send a signal
//using SendSignal and an Event
//TODO(sushi) figure out a better way to send signals than using an Enum. Maybe string but we can't switch on strings so
struct Sender {
    std::set<Receiver*> receivers;
	
    void AddReceiver(Receiver* r);
    void RemoveReceiver(Receiver* r);
    void SendEvent(Event event);
    
    bool HasReceiver(Receiver* r);
};

struct Receiver {
    Receiver(Sender* s = 0);
    virtual void ReceiveEvent(Event event) = 0;
};

#endif //DESHI_EVENT_H