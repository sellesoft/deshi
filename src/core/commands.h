#pragma once
#ifndef DESHI_COMMANDS_H
#define DESHI_COMMANDS_H

#include "kigu/common.h"
#include "kigu/unicode.h"

enum CmdArgument{
	CmdArgument_NONE,
	CmdArgument_S32,
	CmdArgument_String,
	
	CmdArgument_OPTIONAL = (1 << 10),
};

typedef void(*CmdFunc)(str8* args, u32 arg_count);
struct Command{
	CmdFunc func;
	str8 name;
	str8 desc;
	str8 usage;
	u32 min_args;
	u32 max_args;
	Type* args;
	u32 arg_count;
};

struct Alias{
	str8 alias;
	str8 actual;
};

void cmd_add(CmdFunc func, str8 name, str8 desc, Type* args, u32 arg_count);
void cmd_run(str8 input);
void cmd_init();

#endif //DESHI_COMMANDS_H