#include "deshi.h"

int main() {
	deshi::init();

	//game init

	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		
		DeshiImGui::NewFrame();                                                         //place imgui calls after this
		TIMER_RESET(t_d); DengTime->Update();                        DengTime->timeTime = TIMER_END(t_d);
		TIMER_RESET(t_d); DengWindow->Update();                      DengTime->windowTime = TIMER_END(t_d);
		TIMER_RESET(t_d); DengInput->Update();                       DengTime->inputTime = TIMER_END(t_d);
		TIMER_RESET(t_d); DengConsole->Update(); Console2::Update(); DengTime->consoleTime = TIMER_END(t_d);
		TIMER_RESET(t_d); Render::Update();                          DengTime->renderTime = TIMER_END(t_d);  //place imgui calls before this
		UI::Update();
		DengTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}

	deshi::cleanup();
}