#include "event.h"

void Sender::AddReceiver(Receiver* r) {
    receivers.push_back(r);
}

void Sender::RemoveReceiver(Receiver* r) {
    int index = 0;
    for (Receiver* re : receivers) {
        if (r == re) receivers.erase(receivers.begin() + index);
        index++;
    }
}

void Sender::SendEvent(Event event) {
    for (Receiver* r : receivers) {
        r->ReceiveEvent(event);
    }
}

Receiver::Receiver() {
    sender = nullptr;
}

Receiver::Receiver(Sender* s) {
    s->AddReceiver(this);
}

Receiver::~Receiver() {
    sender->RemoveReceiver(this);
}