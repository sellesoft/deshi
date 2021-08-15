#pragma once
#ifndef DESHI_COMMAND_H
#define DESHI_COMMAND_H

//TODO(delle) make this standalone

#include <string>
#include <vector>

struct Admin;

typedef std::string (*CommandAction)(Admin* admin, std::vector<std::string> args);

struct Command{
	CommandAction action;
	bool triggered;
	std::string name;
	std::string description;
	
	Command(CommandAction action, std::string name, std::string description = ""){
		this->triggered = false;
		this->action = action;
		this->name = name;
		this->description = description;
	}
	
	inline std::vector<std::string> ParseArgs(std::string args){
		std::vector<std::string> argsl;
		
		while(args.size() != 0){
			if (args[0] == ' ') {
				args.erase(0, 1);
			}else{
				size_t fc = args.find_last_not_of(" ");//first char
				size_t fs = args.find_first_of(" ");    //first space
				argsl.push_back(args.substr(args.find_last_not_of(" "), args.find_first_of(" ")));
				args.erase(fc, fs - fc);
			}
		}
		return argsl;
	}
	
	//execute command action function
	inline std::string Exec(Admin* admin, std::string args = ""){
		std::vector<std::string> argsl = ParseArgs(args);
		return action(admin, argsl);
	}
};

#endif //DESHI_COMMAND_H