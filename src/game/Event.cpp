#include "event.h"

void Sender::AddReceiver(Receiver* r) {
	if (r) receivers.insert(r);
}

void Sender::RemoveReceiver(Receiver* r) {
	if(r) receivers.erase(r);
}

void Sender::SendEvent(Event event) {
	for (Receiver* r : receivers) {
		r->ReceiveEvent(event);
	}
}

bool Sender::HasReceiver(Receiver* r) {
	if (receivers.find(r) != receivers.end()) return true;
	return false;
}

Receiver::Receiver(Sender* s) {
	if(s) s->AddReceiver(this);
}
