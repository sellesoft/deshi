#include "Keybinds.h"

#include "../EntityAdmin.h"
#include "../systems/ConsoleSystem.h"

#include <fstream>

Keybinds::Keybinds(EntityAdmin* a) : Component(a) {
	
	//push back keys here
	//they must be pushed back in the same order they're defined in the keybinds config
	keys.push_back(&movementFlyingUp);
	keys.push_back(&movementFlyingDown);
	keys.push_back(&movementFlyingForward);
	keys.push_back(&movementFlyingBack);
	keys.push_back(&movementFlyingRight);
	keys.push_back(&movementFlyingLeft);
	
	keys.push_back(&cameraRotateUp);
	keys.push_back(&cameraRotateDown);
	keys.push_back(&cameraRotateRight);
	keys.push_back(&cameraRotateLeft);
	
	keys.push_back(&debugRenderWireframe);
	keys.push_back(&debugRenderEdgesNumbers);
	keys.push_back(&debugRenderDisplayAxis);
	
	stk = std::map<std::string, Key::Key>{
		{"NONE", Key::NONE},
		{"A", Key::A}, {"B", Key::B}, {"C", Key::C}, {"D", Key::D}, {"E", Key::E}, {"F", Key::F}, {"G", Key::G}, {"H", Key::H}, {"I", Key::I}, {"J", Key::J}, {"K", Key::K}, {"L", Key::L}, {"M", Key::M}, {"N", Key::N}, {"O", Key::O}, {"P", Key::P}, {"Q", Key::Q}, {"R", Key::R}, {"S", Key::S}, {"T", Key::T}, {"U", Key::U}, {"V", Key::V}, {"W", Key::W}, {"X", Key::X}, {"Y", Key::Y}, {"Z", Key::Z},
		{"K0", Key::K0}, {"K1", Key::K1}, {"K2", Key::K2}, {"K3", Key::K3}, {"K4", Key::K4}, {"K5", Key::K5}, {"K6", Key::K6}, {"K7", Key::K7}, {"K8", Key::K8}, {"K9", Key::K9},
		{"F1", Key::F1}, {"F2", Key::F2}, {"F3", Key::F3}, {"F4", Key::F4}, {"F5", Key::F5}, {"F6", Key::F6}, {"F7", Key::F7}, {"F8", Key::F8}, {"F9", Key::F9}, {"F10", Key::F10}, {"F11", Key::F11}, {"F12", Key::F12},
		{"UP", Key::UP}, {"DOWN", Key::DOWN}, {"LEFT", Key::LEFT}, {"RIGHT", Key::RIGHT},
		{"ESCAPE", Key::ESCAPE}, {"TILDE", Key::TILDE}, {"TAB", Key::TAB}, {"CAPSLOCK", Key::CAPSLOCK}, {"LSHIFT", Key::LSHIFT}, {"LCONTROL", Key::LCONTROL}, {"LALT", Key::LALT},
		{"BACKSPACE", Key::BACKSPACE}, {"ENTER", Key::ENTER}, {"RSHIFT", Key::RSHIFT}, {"RCONTROL", Key::RCONTROL}, {"RALT", Key::RALT}, {"MINUS", Key::MINUS}, {"EQUALS", Key::EQUALS}, {"LBRACKET", Key::LBRACKET}, {"RBRACKET", Key::RBRACKET},
		{"SLASH", Key::SLASH}, {"SEMICOLON", Key::SEMICOLON}, {"APOSTROPHE", Key::APOSTROPHE}, {"COMMA", Key::COMMA}, {"PERIOD", Key::PERIOD}, {"BACKSLASH", Key::BACKSLASH}, {"SPACE", Key::SPACE},
		{"INSERT", Key::INSERT}, {"DELETE", Key::DELETE}, {"HOME", Key::HOME}, {"END", Key::END}, {"PAGEUP", Key::PAGEUP}, {"PAGEDOWN", Key::PAGEDOWN}, {"PAUSE", Key::PAUSE}, {"SCROLL", Key::SCROLL},
		{"NUMPAD0", Key::NUMPAD0}, {"NUMPAD1", Key::NUMPAD1}, {"NUMPAD2", Key::NUMPAD2}, {"NUMPAD3", Key::NUMPAD3}, {"NUMPAD4", Key::NUMPAD4}, {"NUMPAD5", Key::NUMPAD5}, {"NUMPAD6", Key::NUMPAD6}, {"NUMPAD7", Key::NUMPAD7}, {"NUMPAD8", Key::NUMPAD8}, {"NUMPAD9", Key::NUMPAD9},
		{"NUMPADMULTIPLY", Key::NUMPADMULTIPLY}, {"NUMPADDIVIDE", Key::NUMPADDIVIDE}, {"NUMPADPLUS", Key::NUMPADPLUS}, {"NUMPADMINUS", Key::NUMPADMINUS}, {"NUMPADPERIOD", Key::NUMPADPERIOD}, {"NUMPADENTER", Key::NUMPADENTER}, {"NUMLOCK", Key::NUMLOCK}
	};
	
	//read keys from keybinds.txt
	std::fstream kf;
	kf.open("configs/keybinds.txt", std::fstream::in);
	
	int iter = 0;
	
	//TODO(i, sushi) implement keybind name mapping and shtuff
	
	//there is prolly a better way to do this but uh
	while (!kf.eof()) {
		char* c = (char*)malloc(255); 
		kf.getline(c, 255);
		
		std::string s(c);
		
		if (s[0] == '>') {
			continue;
		}
		else{
			std::string key;
			try {
				key = s.substr(s.find_first_of('=') + 2);
				*keys[iter] = stk.at(key);
				
			}
			catch (const std::out_of_range& oor) {
				ERROR("Key not found in map: ", oor.what());
				//*keys[iter] = Key::NONE;
			}
			
			iter++;
		}
		
		free(c);
	}
	
	
}