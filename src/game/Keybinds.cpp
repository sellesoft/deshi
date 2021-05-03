#include "Keybinds.h"
#include "../core/console.h"
#include "../core/assets.h"

#include <fstream>
#include <regex>

//TODO(anyone) update this string at some point
std::string default_keybinds = ""
"> Movement Keys\n"
"movementFlyingUp = E\n"
"movementFlyingDown = Q\n"
"movementFlyingForward = W\n"
"movementFlyingBack = S\n"
"movementFlyingRight = D\n"
"movementFlyingLeft = A\n"
"> Camera Keys\n"
"cameraRotateUp = UP\n"
"cameraRotateDown = DOWN\n"
"cameraRotateRight = RIGHT\n"
"cameraRotateLeft = LEFT\n"
"> Debug Menu Keys\n"
"toggleConsole = TILDE\n"
"toggleDebugMenu = LCTRL + TILDE\n"
"toggleDebugBar = LSHIFT + TILDE";


void Keybinds::Init(){
	//set up key map
	keys = std::map<std::string, Key::Key&>{
		{"movementFlyingUp",      movementFlyingUp},
		{"movementFlyingDown",    movementFlyingDown},
		{"movementFlyingForward", movementFlyingForward},
		{"movementFlyingBack",    movementFlyingBack},
		{"movementFlyingRight",   movementFlyingRight},
		{"movementFlyingLeft",    movementFlyingLeft},
		
		{"cameraRotateUp",    cameraRotateUp},
		{"cameraRotateDown",  cameraRotateDown},
		{"cameraRotateRight", cameraRotateRight},
		{"cameraRotateLeft",  cameraRotateLeft},
		
		{"toggleConsole",   toggleConsole},
		{"toggleDebugMenu", toggleDebugMenu},
		{"toggleDebugBar",  toggleDebugBar},
		{"toggleMenuBar",   toggleMenuBar},
		
		{"grabSelectedObject",   grabSelectedObject},
		{"rotateSelectedObject", rotateSelectedObject},
		{"scaleSelectedObject",  scaleSelectedObject},
		
		{"undo", undo},
		{"redo", redo},
		
		{"orthoOffset",       orthoOffset},
		{"orthoZoomIn",       orthoZoomIn},
		{"orthoZoomOut",      orthoZoomOut},
		{"orthoResetOffset",  orthoResetOffset},
		{"perspectiveToggle", perspectiveToggle},
		
		{"orthoRightView",    orthoRightView}, 
		{"orthoLeftView",     orthoLeftView}, 
		{"orthoFrontView",    orthoFrontView}, 
		{"orthoBackView",     orthoBackView}, 
		{"orthoTopDownView",  orthoTopDownView}, 
		{"orthoBottomUpView", orthoBottomUpView}
		
	};
	
	//string to key map
	stk = std::map<std::string, Key::Key>{
		{"NONE", Key::NONE},
		{"A", Key::A}, {"B", Key::B}, {"C", Key::C}, {"D", Key::D}, {"E", Key::E}, {"F", Key::F}, {"G", Key::G}, {"H", Key::H}, {"I", Key::I}, {"J", Key::J}, {"K", Key::K}, {"L", Key::L}, {"M", Key::M}, {"N", Key::N}, {"O", Key::O}, {"P", Key::P}, {"Q", Key::Q}, {"R", Key::R}, {"S", Key::S}, {"T", Key::T}, {"U", Key::U}, {"V", Key::V}, {"W", Key::W}, {"X", Key::X}, {"Y", Key::Y}, {"Z", Key::Z},
		{"K0", Key::K0}, {"K1", Key::K1}, {"K2", Key::K2}, {"K3", Key::K3}, {"K4", Key::K4}, {"K5", Key::K5}, {"K6", Key::K6}, {"K7", Key::K7}, {"K8", Key::K8}, {"K9", Key::K9},
		{"F1", Key::F1}, {"F2", Key::F2}, {"F3", Key::F3}, {"F4", Key::F4}, {"F5", Key::F5}, {"F6", Key::F6}, {"F7", Key::F7}, {"F8", Key::F8}, {"F9", Key::F9}, {"F10", Key::F10}, {"F11", Key::F11}, {"F12", Key::F12},
		{"UP", Key::UP}, {"DOWN", Key::DOWN}, {"LEFT", Key::LEFT}, {"RIGHT", Key::RIGHT},
		{"ESCAPE", Key::ESCAPE}, {"TILDE", Key::TILDE}, {"TAB", Key::TAB}, {"CAPSLOCK", Key::CAPSLOCK}, {"LSHIFT", INPUTMOD_SHIFT}, {"LCTRL", INPUTMOD_CTRL}, {"LALT", INPUTMOD_ALT},
		{"BACKSPACE", Key::BACKSPACE}, {"ENTER", Key::ENTER}, {"RSHIFT", INPUTMOD_SHIFT}, {"RCTRL", INPUTMOD_CTRL}, {"RALT", INPUTMOD_ALT}, {"MINUS", Key::MINUS}, {"EQUALS", Key::EQUALS}, {"LBRACKET", Key::LBRACKET}, {"RBRACKET", Key::RBRACKET},
		{"SLASH", Key::SLASH}, {"SEMICOLON", Key::SEMICOLON}, {"APOSTROPHE", Key::APOSTROPHE}, {"COMMA", Key::COMMA}, {"PERIOD", Key::PERIOD}, {"BACKSLASH", Key::BACKSLASH}, {"SPACE", Key::SPACE},
		{"INSERT", Key::INSERT}, {"DELETE", Key::DELETE}, {"HOME", Key::HOME}, {"END", Key::END}, {"PAGEUP", Key::PAGEUP}, {"PAGEDOWN", Key::PAGEDOWN}, {"PAUSE", Key::PAUSE}, {"SCROLL", Key::SCROLL},
		{"NUMPAD0", Key::NUMPAD0}, {"NUMPAD1", Key::NUMPAD1}, {"NUMPAD2", Key::NUMPAD2}, {"NUMPAD3", Key::NUMPAD3}, {"NUMPAD4", Key::NUMPAD4}, {"NUMPAD5", Key::NUMPAD5}, {"NUMPAD6", Key::NUMPAD6}, {"NUMPAD7", Key::NUMPAD7}, {"NUMPAD8", Key::NUMPAD8}, {"NUMPAD9", Key::NUMPAD9},
		{"NUMPADMULTIPLY", Key::NUMPADMULTIPLY}, {"NUMPADDIVIDE", Key::NUMPADDIVIDE}, {"NUMPADPLUS", Key::NUMPADPLUS}, {"NUMPADMINUS", Key::NUMPADMINUS}, {"NUMPADPERIOD", Key::NUMPADPERIOD}, {"NUMPADENTER", Key::NUMPADENTER}, {"NUMLOCK", Key::NUMLOCK},
		{"MBLEFT", Key::MBLEFT}, {"MBRIGHT", Key:: MBRIGHT}, {"MBMIDDLE", Key:: MBMIDDLE}, {"MBFOUR", Key:: MBFOUR}, {" MBFIVE", Key:: MBFIVE}, {"MBSIX", Key:: MBSIX}, {"MBSEVEN", Key:: MBSEVEN}, {"MBEIGHT", Key:: MBEIGHT}, {"MBSCROLLDOWN", Key:: MBSCROLLDOWN}, {"MBSCROLLUP", Key:: MBSCROLLUP} 
	};
	
	//read keys from keybinds.cfg
	std::fstream kf;
	kf.open(deshi::dirConfig() + "keybinds.cfg", std::fstream::in);
	if (kf.is_open()) { //file exists
		ReloadKeybinds(kf);
		kf.close();
	}else{              //file doesnt exist
		kf.close();
		LOG("No keybinds file found, generating a new one in the cfg directory");
		deshi::writeFile(deshi::dirConfig() + "keybinds.cfg", default_keybinds.c_str(), default_keybinds.size());
		kf.open(deshi::dirConfig() + "keybinds.cfg", std::fstream::in);
		ReloadKeybinds(kf);
		kf.close();
	}
}
void Keybinds::ReloadKeybinds(std::fstream& kf) {
	//TODO( sushi,In) implement keybind name mapping and shtuff
	
	//there is prolly a better way to do this but hehe
	//NOTE IF YOU ARE GOING TO IMPLEMENT RIGHT/LEFT MOD DIFFERENCE AND ITS NOT WORKING MAKE
	//SURE YOU CHANGE THE ABOVE MAP TO WORK WITH IT !!!
	int line = 0;
	while (!kf.eof()) {
		char* c = (char*)malloc(255);
		kf.getline(c, 255);
		std::string s(c);
		
		if (s[0] == '>') line++; continue; //skip comments
		
		if (s != "") {
			std::smatch m;
			std::regex r;
			bool modif = false;
			
			//if string contains a + it must have a modifier
			if(s.find("+") != std::string::npos) {
				r = std::regex("([A-Za-z]+) += +(LCTRL|LSHIFT|LALT|RCTRL|RSHIFT|RALT) +\\+ +(.+)");
				modif = true;
			}else{
				r = std::regex("([A-Za-z]+) += +(.+)");
			}
			
			std::regex_match(s, m, r);
			if (m.size() == 1) {
				ERROR_LOC(m[1], "\nRegex did not find any matches for this string in keybinds.txt at line ", line);
				line++; continue;
			}
			
			try {
				if(modif) {
					try {
						keys.at(m[1]) = stk.at(m[3]) | stk.at(m[2]);
					}catch(std::out_of_range oor) {
						ERROR_LOC("Either the modifier \"", m[2], "\", the keybind name \"", m[3], "\", or the key name \"", m[1], "\" was not found in their respective maps. \nAt line ", line, " in keybinds.txt");
					}
				}else{
					try {
						keys.at(m[1]) = stk.at(m[2]);
					}catch (std::out_of_range oor) {
						ERROR_LOC("Either the keybind \"", m[1], "\" or the key \"", m[2], "\" was not found in their respective maps. \nAt line ", line, " in keybinds.txt");
					}
				}
			}catch(...) {
				ERROR_LOC("Key \"", m[1], "\" not found in keymap. \nAt line ", line, " in keybinds.txt");
			}
			line++;
			free(c);
		}
	} //while (!kf.eof())
}

