namespace Cmd{
    ///////////////////
    //// @internal ////
    ///////////////////
    array<Command> commands;
    array<Alias> aliases;
    char* last_cmd_desc;
    
    void AddDeshiCommands(){
#define CMDSTART(name, desc) last_cmd_desc = desc; auto deshi__cmd__##name = [](array<cstring>& args) -> void
#define CMDEND(name, ...) ; Add(deshi__cmd__##name, #name, last_cmd_desc, {__VA_ARGS__});
        
        CMDSTART(add, "Adds two numbers together"){
            if(args.count != 2){
                Console2::Log("Error: 'add' requires exactly 2 arguments");
                return;
            }
            
            int i0 = atoi(args[0].str);
            int i1 = atoi(args[1].str);
            Console2::Log(TOSTRING(i0," + ",i1," = ", i0+i1));
        }CMDEND(add, Argument_S32, Argument_S32);
        
        CMDSTART(daytime, "Logs the time in day-time format"){
            Console2::Log(DeshTime->FormatDateTime("{w} {M}/{d}/{y} {h}:{m}:{s}").c_str());
        }CMDEND(daytime);
        
        CMDSTART(time_engine, "Logs deshi engine times"){
            Console2::Log(DeshTime->FormatTickTime("Time:   {t}ms Window:{w}ms Input:{i}ms Admin:{a}ms\n"
                                                   "Console:{c}ms Render:{r}ms Frame:{f}ms Delta:{d}ms").c_str());
        }CMDEND(time_engine);
        
        CMDSTART(list, "Lists available commands"){
            string commands_list;
            forE(commands){
                commands_list += it->name;
                commands_list += ": ";
                commands_list += it->desc;
                commands_list += "\n";
            }
            Console2::Log(commands_list);
        }CMDEND(list);
        
        CMDSTART(help, "Logs description and usage of specified command"){
            if(args.count == 0){
                Console2::Log("Use 'help <command>' to get a description and usage of the command");
                Console2::Log("Usage Format: command <required> [optional]");
                return;
            }
            bool found = false;
            forE(commands){
                if(strcmp(it->name.str, args[0].str) == 0){
                    Console2::Log(it->desc);
                    Console2::Log(it->usage);
                    found = true;
                    break;
                }
            }
            if(!found){
                Console2::Log(TOSTRING("Error: Command '",args[0],"' not found"));
            }
        }CMDEND(help, Argument_String|Argument_OPTIONAL);
        
        CMDSTART(alias, "Gives an alias to specified command and arguments"){
            //check that alias name and command arent the same
            if(args[0] == args[1]){ //!Robustness: this check doesnt compare inside args[1], so an alias could still be recursive
                Console2::Log("Error: Aliases can't be recursive");
                return;
            }
            
            //check if name is used by a command
            forE(commands){
                if(strncmp(it->name.str, args[0].str, it->name.size) == 0){
                    Console2::Log("Error: Aliases can't use the same name as an existing command");
                    return;
                }
            }
            
            //check if alias already exists
            u32 idx = -1;
            forE(aliases){
                if(strcmp(it->name.str, args[0].str) == 0){
                    idx = it-it_begin;
                    break;
                }
            }
            if(idx == -1){
                aliases.add({to_string(args[0]), to_string(args[1])});
            }else{
                aliases[idx].command = to_string(args[1]);
            }
        }CMDEND(alias, Argument_String, Argument_String);
        
        CMDSTART(aliases, "Lists available aliases"){
            forE(aliases){
                Console2::Log(TOSTRING(it->name,": ",it->command));
            }
        }CMDEND(aliases);
        
        CMDSTART(window_display_mode, "Changes whether the window is in windowed(0), borderless(1), or fullscreen(2) mode"){
            int mode = atoi(args[0].str);
            switch(mode){
                case 0:{
                    DeshWindow->UpdateDisplayMode(DisplayMode_Windowed);
                    Console2::Log("Window display mode updated to 'windowed'");
                }break;
                case 1:{
                    DeshWindow->UpdateDisplayMode(DisplayMode_Borderless);
                    Console2::Log("Window display mode updated to 'borderless'");
                }break;
                case 2:{
                    DeshWindow->UpdateDisplayMode(DisplayMode_Fullscreen);
                    Console2::Log("Window display mode updated to 'fullscreen'");
                }break;
                default:{
                    Console2::Log("Display Modes: 0=Windowed, 1=Borderless, 2=Fullscreen");
                }break;
            }
        }CMDEND(window_display_mode, Argument_S32);
        
        CMDSTART(window_cursor_mode, "Changes whether the cursor is in default(0), first person(1), or hidden(2) mode"){
            int mode = atoi(args[0].str);
            switch(mode){
                case 0:{
                    DeshWindow->UpdateCursorMode(CursorMode_Default);
                    Console2::Log("Cursor mode updated to 'default'");
                }break;
                case 1:{
                    DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);
                    Console2::Log("Cursor mode updated to 'first person'");
                }break;
                case 2:{
                    DeshWindow->UpdateCursorMode(CursorMode_Hidden);
                    Console2::Log("Cursor mode updated to 'hidden'");
                }break;
                default:{
                    Console2::Log("Cursor Modes: 0=Default, 1=First Person, 2=Hidden");
                }break;
            }
        }CMDEND(window_cursor_mode, Argument_S32);
        
        CMDSTART(window_raw_input, "Changes whether the window uses raw input"){
            int mode = atoi(args[0].str);
            switch(mode){
                case 0:{
                    DeshWindow->UpdateRawInput(false);
                    Console2::Log("Raw input updated to 'false'");
                }break;
                case 1:{
                    DeshWindow->UpdateRawInput(true);
                    Console2::Log("Raw input updated to 'true'");
                }break;
                default:{
                    Console2::Log("Raw Input: 0=False, 1=True");
                }break;
            }
        }CMDEND(window_raw_input, Argument_S32);
        
        CMDSTART(window_resizable, "Changes whether the window is resizable"){
            int mode = atoi(args[0].str);
            switch(mode){
                case 0:{
                    DeshWindow->UpdateResizable(false);
                    Console2::Log("Window resizability updated to 'false'");
                }break;
                case 1:{
                    DeshWindow->UpdateResizable(true);
                    Console2::Log("Window resizability updated to 'true'");
                }break;
                default:{
                    Console2::Log("Window resizability: 0=False, 1=True");
                }break;
            }
        }CMDEND(window_resizable, Argument_S32);
        
        CMDSTART(window_info, "Lists window's vars"){
            string dispMode;
            switch(DeshWindow->displayMode){
                case(DisplayMode_Windowed):  { dispMode = "Windowed"; }break;
                case(DisplayMode_Borderless):{ dispMode = "Borderless Windowed"; }break;
                case(DisplayMode_Fullscreen):{ dispMode = "Fullscreen"; }break;
            }
            string cursMode;
            switch(DeshWindow->cursorMode){
                case(CursorMode_Default):    { cursMode = "Default"; }break;
                case(CursorMode_FirstPerson):{ cursMode = "First Person"; }break;
                case(CursorMode_Hidden):     { cursMode = "Hidden"; }break;
            }
            Console2::Log(TOSTRING("Window Info"
                                   "\n    Window Position: ",DeshWindow->x,",",DeshWindow->y,
                                   "\n    Window Dimensions: ",DeshWindow->width,"x",DeshWindow->height,
                                   "\n    Screen Dimensions: ",DeshWindow->screenWidth,"x",DeshWindow->screenHeight,
                                   "\n    Refresh Rate: ",DeshWindow->refreshRate,
                                   "\n    Screen Refresh Rate: ",DeshWindow->screenRefreshRate,
                                   "\n    Display Mode: ",dispMode,
                                   "\n    Cursor Mode: ",cursMode,
                                   "\n    Raw Input: ",DeshWindow->rawInput,
                                   "\n    Resizable: ",DeshWindow->resizable,
                                   "\n    Restores: ",DeshWindow->restoreX,",",DeshWindow->restoreY,
                                   " ",DeshWindow->restoreW,"x",DeshWindow->restoreH));
        }CMDEND(window_info);
        
        CMDSTART(mat_list, "Lists the materials and their info"){
            Console2::Log("Material List:"
                          "\nName\tShader\tTextures");
            forI(Storage::MaterialCount()){
                Material* mat = Storage::MaterialAt(i);
                string text = TOSTRING(mat->name,'\t',ShaderStrings[mat->shader],'\t');
                forI(mat->textures.size()){
                    text += ' ';
                    text += Storage::TextureName(mat->textures[i]);
                }
                Console2::Log(text);
            }
        }CMDEND(mat_list);
        
        CMDSTART(mat_texture, "Changes a texture of a material"){
            int matID = atoi(args[0].str);
            int texSlot = atoi(args[1].str);
            int texID = atoi(args[2].str);
            Storage::MaterialAt(matID)->textures[texSlot] = texID;
            Console2::Log(TOSTRING("Updated material ",Storage::MaterialName(matID),"'s texture",texSlot,
                                   " to ",Storage::TextureName(texID)));
        }CMDEND(mat_texture, Argument_S32, Argument_S32, Argument_S32);
        
        CMDSTART(mat_shader, "Changes the shader of a material"){
            int matID = atoi(args[0].str);
            int shader = atoi(args[1].str);
            Storage::MaterialAt(matID)->shader = (Shader)shader;
            Console2::Log(TOSTRING("Updated material ",Storage::MaterialName(matID),"'s shader to ", ShaderStrings[shader]));
        }CMDEND(mat_shader, Argument_S32, Argument_S32);
        
        CMDSTART(shader_reload, "Reloads specified shader"){
            int id = atoi(args[0].str);
            if(id == -1){
                Render::ReloadAllShaders();
                Console2::Log("^c:magen^Reloaded all shaders^c^");
            }else if(id < Shader_COUNT){
                Render::ReloadShader(id);
                Console2::Log(TOSTRING("^c:magen^Reloaded '",ShaderStrings[id],"'^c^"));
            }else{
                Console2::Log(TOSTRING("Error: There is no shader with id: ",id));
            }
        }CMDEND(shader_reload, Argument_S32);
        
        CMDSTART(shader_list, "Lists the shaders and their info"){
            Console2::Log("Shader List:"
                          "\nID\tName");
            forI(ArrayCount(ShaderStrings)){
                Console2::Log(TOSTRING(i,'\t',ShaderStrings[i]));
            }
        }CMDEND(shader_list);
        
        CMDSTART(texture_load, "Loads a specific texture"){
            TIMER_START(t_l);
            TextureType type = TextureType_2D;
            if(args.count == 2) type = (TextureType)atoi(args[1].str);
            Storage::CreateTextureFromFile(to_string(args[0]).str, ImageFormat_RGBA, type);
            Console2::Log(TOSTRING("Loaded texture '",args[0],"' in ",TIMER_END(t_l),"ms"));
        }CMDEND(texture_load, Argument_String, Argument_S32|Argument_OPTIONAL);
        
        CMDSTART(texture_list, "Lists the textures and their info"){
            Console2::Log(TOSTRING("Texture List:"
                                   "\nName\tWidth\tHeight\tDepth\tMipmaps\tType"));
            forI(Storage::TextureCount()){
                Texture* tex = Storage::TextureAt(i);
                Console2::Log(TOSTRING('\n',tex->name,'\t',tex->width,'\t',tex->height,'\t',tex->depth,
                                       '\t',tex->mipmaps,'\t',TextureTypeStrings[tex->type]));
            }
        }CMDEND(texture_list);
        
        CMDSTART(quit, "Exits the application"){
            DeshWindow->Close();
        }CMDEND(quit);
        
#undef CMDSTART
#undef CMDEND
    }
    
    
    ////////////////////
    //// @interface ////
    ////////////////////
    void Add(CmdFunc func, const string& name, const string& desc, const array<Argument>& args){
        u32 min_args = 0;
        u32 max_args = 0;
        string usage = name;
        forE(args){
            max_args++;
            usage += ' ';
            if(*it & Argument_OPTIONAL){
                usage += '[';
            }else{
                usage += '<';
                min_args++;
            }
            
            if      (*it & Argument_S32){
                usage += "S32";
            }else if(*it & Argument_String){
                usage += "String";
            }
            
            if(*it & Argument_OPTIONAL){
                usage += ']';
            }else{
                usage += '>';
            }
        }
        commands.add({func, name, desc, usage, min_args, max_args, args});
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
            
            cstring word = eat_until_char_skip_quotes(remaining, ' ');
            if(word){
                advance(&remaining, word.count);
                if(word[0] == '\"'){
                    word.str++;
                    word.count-=2;
                }
                args.add(word);
            }else{
                if(remaining[0] == '\"'){
                    remaining.str++;
                    remaining.count-=2;
                }
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
                    if(args.count < it->min_args){
                        Console2::Log(TOSTRING("Error: Command '",command_name,"' requires at least ",it->min_args," arguments"));
                    }else if(args.count > it->max_args){
                        Console2::Log(TOSTRING("Error: Command '",command_name,"' requires at most ",it->max_args," arguments"));
                    }else{
                        it->func(args);
                    }
                }else{
                    Console2::Log(TOSTRING("Error: Command '",command_name,"' has no registered function"));
                }
                found = true;
                break;
            }
        }
        forE(aliases){
            if(command_name == it->name){
                Run(it->command);
                found = true;
                break;
            }
        }
        if(!found){
            Console2::Log(TOSTRING("Error: Unknown command '",command_name,"'"));
        }
    }
}; //namespace Cmd
