#pragma once
#include "Debug.h"

struct EntityAdmin;

typedef std::string (*CommandAction)(EntityAdmin* admin, std::vector<std::string> args);

//typedef void (*CommandActionArgs)(EntityAdmin* admin, std::string args);

struct Command {
	CommandAction action;
	bool triggered;
	std::string name;
	std::string description;

	static bool CONSOLE_PRINT_EXEC;

	Command(CommandAction action, std::string name, std::string description = "") {
		this->triggered = false;
		this->action = action;
		this->name = name;
		this->description = description;
	}

	inline std::vector<std::string> ParseArgs(std::string args) {
		std::vector<std::string> argsl;
		
		while (args.size() != 0) {
			if (args[0] == ' ') {
				args.erase(0, 1);
			}
			else {
				size_t fc = args.find_first_not_of(" ");//first char
				size_t fs = args.find_first_of(" ");    //first space
				argsl.push_back(args.substr(args.find_first_not_of(" "), args.find_first_of(" ")));
				args.erase(fc, fs - fc);
			}

		}
		return argsl;
	}

	//execute command action function
	inline std::string Exec(EntityAdmin* admin, std::string args = "") {
 		std::vector<std::string> argsl = ParseArgs(args);
		//DEBUG if(CONSOLE_PRINT_EXEC) LOG("Executing command: ", name);
		//if(CONSOLE_PRINT_EXEC) Debug::ToString(1, std::string("Executing command: ") + name, true);
		return action(admin, argsl);
	}
};

//TODO(i,delle) maybe add delayed and repeating commands
//struct DelayedCommand : public Command { 
//	float delay;
//
//	DelayedCommand(CommandAction action, std::string name, float delay, std::string description = "") {
//		this->triggered = false;
//		this->action = action;
//		this->name = name;
//		this->description = description;
//	}
//
//	inline void operator () (EntityAdmin* admin) const {
//		action(admin);
//	}
//};