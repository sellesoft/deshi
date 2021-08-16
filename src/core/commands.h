#pragma once
#ifndef DESHI_COMMANDS_H
#define DESHI_COMMANDS_H

#include "../utils/cstring.h"

namespace Cmd{
    typedef void(*CmdFunc)(array<cstring>& args);
    struct CommandInfo{
        CmdFunc func;
        string name;
        string desc;
        string usage;
    };
    
    void Init();
    void Add(CmdFunc func, const string& name, const string& desc, const string& usage);
    void Run(const string& input);
};

#endif //DESHI_COMMANDS_H
