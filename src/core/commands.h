#pragma once
#ifndef DESHI_COMMANDS_H
#define DESHI_COMMANDS_H

#include "../defines.h"
#include "../utils/string.h"
#include "../utils/cstring.h"

enum CmdArgument{
	CmdArgument_NONE,
	CmdArgument_S32,
	CmdArgument_String,
	
	CmdArgument_OPTIONAL = 1024,
};

namespace Cmd{
	typedef void(*CmdFunc)(array<cstring>& args);
	struct Command{
		CmdFunc func;
		string name;
		string desc;
		string usage;
		u32 min_args;
		u32 max_args;
		array<Type> args;
	};
	
	struct Alias{
		string name;
		string command;
	};
	
	void Init();
	void Add(CmdFunc func, const string& name, const string& desc, const array<Type>& args);
	void Run(const string& input);
};

#endif //DESHI_COMMANDS_H
