#pragma once
#ifndef DESHI_COMMANDS_H
#define DESHI_COMMANDS_H

#include "../utils/cstring.h"

namespace Cmd{
	enum Argument_{
		Argument_NONE,
		Argument_S32,
		Argument_String,
		
		Argument_OPTIONAL = 1024,
	}; typedef u32 Argument;
	
	typedef void(*CmdFunc)(array<cstring>& args);
	struct Command{
		CmdFunc func;
		string name;
		string desc;
		string usage;
		u32 min_args;
		u32 max_args;
		array<Argument> args;
	};
	
	struct Alias{
		string name;
		string command;
	};
	
	void Init();
	void Add(CmdFunc func, const string& name, const string& desc, const array<Argument>& args);
	void Run(const string& input);
};

#endif //DESHI_COMMANDS_H
