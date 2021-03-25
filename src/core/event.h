#pragma once

#ifndef DESHI_EVENT_H
#define DESHI_EVENT_H

#include "../core.h"


enum Event {
    TEST_EVENT
};


struct Receiver;

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

    Sender* sender;

    virtual void ReceiveEvent(Event event) = 0;
};

#endif