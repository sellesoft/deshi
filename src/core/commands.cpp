namespace Cmd{
    ///////////////////
    //// @internal ////
    ///////////////////
    array<CommandInfo> commands;
    char* last_cmd_usage;
    
    void AddDeshiCommands(){
#define CMDSTART(name, usage) last_cmd_usage = usage; auto deshi__cmd__##name = [](array<cstring>& args) -> void
#define CMDEND(name, desc) ; commands.add({deshi__cmd__##name, #name, desc, last_cmd_usage})
        
        CMDSTART(add, "add <int> <int>"){
            if(args.count != 2){
                Console2::Log("Error: 'add' requires exactly 2 arguments");
                return;
            }
            
            int i0 = atoi(args[0].str);
            int i1 = atoi(args[1].str);
            Console2::Log(TOSTRING(i0," + ",i1," = ", i0+i1));
        }CMDEND(add, "Adds two numbers together");
        
        CMDSTART(daytime, "daytime"){
            Console2::Log(DeshTime->FormatDateTime("{w} {M}/{d}/{y} {h}:{m}:{s}").c_str());
        }CMDEND(daytime, "");
        
        CMDSTART(time_engine, "time_engine"){
            Console2::Log(DeshTime->FormatTickTime("Time:   {t}ms Window:{w}ms Input:{i}ms Admin:{a}ms\n"
                                                   "Console:{c}ms Render:{r}ms Frame:{f}ms Delta:{d}ms").c_str());
        }CMDEND(time_engine, "");
        
        CMDSTART(list, "list"){
            string commands_list;
            forE(commands){
                commands_list += it->name;
                commands_list += "\n";
            }
            Console2::Log(commands_list);
        }CMDEND(list, "");
        
        CMDSTART(help, "help <command>"){
            if(args.count == 0){
                Console2::Log("Use 'help <command>' to get a description and usage of the command");
                return;
            }
            
        }CMDEND(help, "");
        
#undef CMDSTART
#undef CMDEND
    }
    
    
    ////////////////////
    //// @interface ////
    ////////////////////
    void Add(CmdFunc func, const string& name, const string& desc, const string& usage){
        commands.add({func, name, desc, usage});
    }
    
    void Init(){
        AddDeshiCommands();
    }
    
    void Run(const string& input){
        cstring remaining{input.str, input.size};
        array<cstring> args;
        
        while(remaining){
            remaining = eat_spaces(remaining);
            if(!remaining) break;
            
            cstring word = eat_until_char(remaining, ' ');
            if(word){
                advance(&remaining, word.count);
                args.add(word);
            }else{
                args.add(remaining);
                break;
            }
        }
        if(!args.count) return;
        
        string command_name = string(args[0].str, args[0].count);
        args.remove(0);
        bool found = false;
        forE(commands){
            if(command_name == it->name){
                if(it->func){
                    it->func(args);
                }else{
                    Console2::Log(TOSTRING("Error: Command '",command_name,"' has no registered function"));
                }
                found = true;
                break;
            }
        }
        if(!found){
            Console2::Log(TOSTRING("Error: Unknown command '",command_name,"'"));
        }
    }
}; //namespace Cmd