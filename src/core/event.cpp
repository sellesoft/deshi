#include "event.h"

void Sender::AddReceiver(Receiver* r) {
    if(r) receivers.push_back(r);
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

Receiver::Receiver(Sender* s) {
    if(s) s->AddReceiver(this);
}
