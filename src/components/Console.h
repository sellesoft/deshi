#pragma once
#include "Component.h"
#include "../utils/Color.h"

struct Console : public Component {
	char inputBuf[256]{};
	std::vector<std::pair<std::string, Color>> buffer; //text, color
	std::vector<std::string> history;
	int historyPos = -1;

	Console();

	bool autoScroll = true;
	bool scrollToBottom = false;

};