#pragma once

#ifndef DESHI_EVENT_H
#define DESHI_EVENT_H

#include "../core.h"


enum Event {
    TEST_EVENT
};


struct Receiver;

//object stored on a component that wants to send a signal
//using SendSignal and an Event
//TODO(sushi) figure out a better way to send signals than using an Enum. Maybe string but we can't switch on strings so
struct Sender {
    std::vector<Receiver*> receivers;
    void AddReceiver(Receiver* r);
    void RemoveReceiver(Receiver* r);
    void SendEvent(Event event);
};

struct Receiver {

    Receiver();
    Receiver(Sender* s);
    ~Receiver();

    Sender* send;

    virtual void ReceiveEvent(Event event) = 0;
};

#endif