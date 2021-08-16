namespace Console2{
    ////////////////////////////////////
    //// internal console variables ////
    ////////////////////////////////////
    local f32 open_max_percent = 0.7f; //percentage of the height of the window to open to
    local f32 open_amount      = 0.0f; //current opened amount
    local f32 open_target      = 0.0f; //target opened amount
    local f32 open_dt          = 2000.0f; //speed at which it opens
    
    local f32 console_x = 0.0f;
    local f32 console_y = 0.0f;
    local f32 console_w = 0.0f;
    local f32 console_h = 0.0f;
    
    local f32 pad   = 5.0f;
    local f32 pad2x = 10.0f;
    local f32 input_box_height = 20.0f;
    
    local f32 font_width  = 0.0f;
    local f32 font_height = 0.0f;
    
    local bool scroll_to_bottom = false;
    local bool show_autocomplete = false;
    
    local f32 console_scroll_y = 0;
    local u32 console_rows_in_buffer = 0; 
    
    local ConsoleState state = ConsoleState_Closed;
    local ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    
    local constexpr u32 input_max_size = 256;
    local constexpr u32 input_history_length = 8;
    
    local char input_buffer[input_max_size] = {0};
    local char input_history[input_max_size*input_history_length] = {0};
    local u32  input_history_index = 0;
    local u32  input_history_select_index = 0;
    
    struct ColoredPstring{
        color color;
        char* start;
        char* end;
    };
    local ring_array<char>           history;
    local ring_array<ColoredPstring> dictionary;
    
    local map<const char*, color> color_strings{
        {"red",    color::RED},     {"dred",    color::DARK_RED},
        {"blue",   color::BLUE},    {"dblue",   color::DARK_BLUE},
        {"cyan",   color::CYAN},    {"dcyan",   color::DARK_CYAN},
        {"grey",   color::GREY},    {"dgrey",   color::DARK_GREY},
        {"green",  color::GREEN},   {"dgreen",  color::DARK_GREEN},
        {"yellow", color::YELLOW},  {"dyellow", color::DARK_YELLOW},
        {"magen",  color::MAGENTA}, {"dmagen",  color::DARK_MAGENTA},
        {"white",  color::WHITE},   {"black",   color::BLACK}
    };
    
    
    ////////////////////////////
    //// internal functions ////
    ////////////////////////////
    
    local void FlushBuffer(){
        //!Incomplete
    }
    
    local void UpdateOpenness(){
        f32 delta_open = DeshTime->deltaTime * open_dt;
        
        if(open_amount < open_target){
            open_amount += delta_open;
            if(open_amount > open_target) open_amount = open_target;
        }else if(open_amount > open_target){
            open_amount -= delta_open;
            if(open_amount < 0) open_amount = 0;
        }
    }
    
    local int TextEditCallback(ImGuiInputTextCallbackData* data) {
        return 0;
    }
    
    /////////////////////////////
    //// interface functions ////
    /////////////////////////////
    bool IsOpen(){
        return open_target > 0;
    }
    
    void Toggle(ConsoleState new_state){
        window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        if(state == new_state) new_state = ConsoleState_Closed;
        intercepting_inputs = true;
        
        switch(new_state){
            case ConsoleState_Closed:{
                state = ConsoleState_Closed;
                open_target = 0;
                intercepting_inputs = false;
            }break;
            case ConsoleState_OpenSmall:{
                state = ConsoleState_OpenSmall;
                open_target = (f32)DeshWindow->height * 0.2f;
            }break;
            case ConsoleState_OpenBig:{
                state = ConsoleState_OpenBig;
                open_target = (f32)DeshWindow->height * open_max_percent;
            }break;
            case ConsoleState_Popout:{
                state = ConsoleState_Popout;
                //!Incomplete
                //create a moveable and resizable ImGui-style window
            }break;
            case ConsoleState_Window:{
                state = ConsoleState_Window;
                //!Incomplete
                //create a new GLFW window child of the deshi window
            }break;
        }
    }
    
    //    ^c=cyan^/^c^^c=dcyan^\\^c^ - reference
    //    ^c=red^red^c^white^c=blue^blue^c^
    //TODO(delle) this might break if the string doesnt end with \n
    void Log(string message){
        message += "\n";
        
        int special_start_idx = -1, special_stop_idx = -1;
        int color_start_idx = -1, color_stop_idx = -1;
        color color_color;
        int chunk_start = 0;
        
        for(int i = 0; i < message.size; ++i){
            //check for special
            if(message[i] == '^'){
                if(special_start_idx != -1){
                    special_stop_idx = i;
                    chunk_start = i+1;
                }else{
                    special_start_idx = i;
                    
                    if((color_start_idx == -1) && (i != chunk_start)){
                        u32 idx = history.count;
                        history.add(message.str+chunk_start, i-chunk_start);
                        dictionary.add(ColoredPstring{color::WHITE, history.at(idx), history.at(idx)+(i-chunk_start)});
                    }
                }
            }
            
            //parse special
            if(special_stop_idx != -1){
                string special_text = message.substr(special_start_idx + 1, special_stop_idx - 1);
                if(special_text[0] == 'c'){ //color special
                    if(color_start_idx == -1){
                        color_start_idx = special_stop_idx+1;
                        
                        if(special_text.size > 2){
                            if(color* c = color_strings.at(special_text.str+2)){
                                color_color = *c;
                            }
                        }
                    }else{
                        color_stop_idx = special_start_idx;
                        
                        if(color_stop_idx != color_start_idx){
                            u32 idx = history.count;
                            history.add(message.str+color_start_idx, color_stop_idx-color_start_idx);
                            dictionary.add(ColoredPstring{color_color, history.at(idx), history.at(idx)+(color_stop_idx-color_start_idx)});
                        }
                        
                        color_start_idx = -1;
                        color_stop_idx = -1;
                    }
                }else if(special_text[0] == 'a'){ //alert special
                    
                    
                }else{ //unhandled special
                    
                }
                
                special_start_idx = -1;
                special_stop_idx = -1;
            }
            
            if(message[i] == '\n'){
                if((i-chunk_start+1 - chunk_start) != 0){
                    u32 idx = history.count;
                    history.add(message.str+chunk_start, (i-chunk_start)+2); //+2 to copy the \0 as well
                    dictionary.add(ColoredPstring{color::WHITE, history.at(idx), history.at(idx)+((i-chunk_start)+1)});
                }
            }
        }
    }
    
    void Init(){
        history.init(8192);
        dictionary.init(512);
    }
    
    void Cleanup(){
        FlushBuffer();
    }
    
    local bool test_swap = false;
    void Update(){
        {//// handle inputs ////
            //open and close console
            if(DeshInput->KeyPressedAnyMod(Key::F1)){
                if(DeshInput->LShiftDown()){
                    Toggle(ConsoleState_OpenBig);
                }else if(DeshInput->LAltDown()){
                    test_swap = !test_swap;
                }else{
                    Toggle(ConsoleState_OpenSmall);
                }
            }
            
            if(intercepting_inputs){
                //!Incomplete
                //scrolling
                if(DeshInput->KeyDownAnyMod(MouseButton::SCROLLUP)) {
                    //console_scroll_y--;
                }
                if(DeshInput->KeyDownAnyMod(MouseButton::SCROLLDOWN)) {
                    //console_scroll_y++;
                }
                
                //!Incomplete
                //input history
                if(DeshInput->KeyDownAnyMod(Key::UP)) {
                    input_history_select_index += 1;
                    if(input_history_select_index > input_history_length) input_history_select_index = 0;
                    if(input_history_select_index) {
                        memcpy(input_history, input_history+(input_history_select_index*input_max_size), input_max_size);
                    }else{
                        memset(input_history, 0, input_max_size);
                    }
                }
                if(DeshInput->KeyDownAnyMod(Key::DOWN)) {
                    input_history_select_index -= 1;
                    if(input_history_select_index < 0) input_history_select_index = input_history_length;
                    if(input_history_select_index) {
                        memcpy(input_history, input_history+(input_history_select_index*input_max_size), input_max_size);
                    }else{
                        memset(input_history, 0, input_max_size);
                    }
                }
            }
        }
        {//// draw console ////
            UpdateOpenness(); 
            console_w = (f32)DeshWindow->width;
            console_h = open_amount;
            if(!open_amount) return; //early out if fully closed
            
            if(test_swap){
                Render::FillRectUI(vec2(console_x, console_y), vec2(console_w, console_h), color::BLACK); //background
                Render::FillRectUI(vec2(console_x+pad, console_y+pad), vec2(console_w-pad2x, console_h-pad2x-input_box_height), color(0, 9, 13)); //report
                Render::FillRectUI(vec2(console_x+pad, console_h-pad-input_box_height), vec2(console_w-pad2x, input_box_height), color(0, 62, 62)); //input
            }else{
                ImGuiStyle& style = ImGui::GetStyle();
                style.AntiAliasedFill = false;
                style.AntiAliasedLines = false;
                style.AntiAliasedLinesUseTex = false;
                style.WindowMinSize = ImVec2(1.f,1.f);
                
                ImGuiIO& io = ImGui::GetIO();
                //io.BackendFlags = ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;
                //io.ConfigWindowsMoveFromTitleBarOnly = true;
                //io.ConfigWindowsResizeFromEdges = true;
                
                font_height = ImGui::GetFontSize();
                font_width = ceil(font_height / 2);
                
                
                ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
                ImGui::PushStyleColor(ImGuiCol_Border,               ImVec4(  0.f,   0.f,   0.f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_TitleBg,              ImVec4(  0.f,   0.f,   0.f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImVec4(  0.f,   0.f,   0.f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        ImVec4(  0.f,   0.f,   0.f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImVec4(.106f, .141f, .141f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(  0.f, .369f, .326f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(.188f, .333f, .353f, 1.f));
                ImGui::SetNextWindowPos(ImVec2(console_x, console_y));
                ImGui::SetNextWindowSize(ImVec2(console_w, console_h));
                
                ImGui::Begin("##console_window", 0, window_flags);{
                    //// history report region ////
                    f32 footer_height_to_reserve = style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(.016f, .067, .082f, 1.f));
                    ImGui::BeginChild("##console_report_region", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);{
                        if(dictionary.count){
                            f32 x = 0;
                            f32 y = ImGui::GetWindowHeight();
                            ImGui::PushTextWrapPos(0.0f);
                            
                            //!Incomplete
                            //handle scrolling
                            
                            u32 str_start = dictionary.end;
                            u32 str_end   = dictionary.end;
                            while(y > 0){
                                //!Incomplete
                                //crashes when going past max size
                                if((str_start != dictionary.start) && (*(dictionary[str_start-1].end - 1) != '\n')) { //skip until string end
                                    str_start -= 1;
                                    if(str_start == -1) str_start = dictionary.capacity-1;
                                    continue;
                                }
                                
                                //calc text y-draw position
                                float string_width = ImGui::CalcTextSize(dictionary[str_start].start, dictionary[str_end].end).x;
                                y -= font_height * ceil(string_width / ImGui::GetWindowWidth());
                                
                                //draw text
                                for(int i = str_start; i <= str_end; ++i){
                                    ImGui::SetCursorPos(ImVec2(x, y));
                                    ImGui::PushStyleColor(ImGuiCol_Text, ColorToVec4(dictionary[i].color));
                                    ImGui::TextEx(dictionary[i].start, dictionary[i].end, ImGuiTextFlags_NoWidthForLargeClippedText);
                                    ImGui::PopStyleColor();
                                    
                                    //!Incomplete
                                    //handle text wrapping
                                    x += ImGui::CalcTextSize(dictionary[i].start, dictionary[i].end).x;
                                    if(x > ImGui::GetWindowWidth()){
                                        y += font_height;
                                        x = 0;
                                    }
                                }
                                x = 0;
                                
                                if(str_start == dictionary.start) break; //end loop after using last element
                                str_start -= 1; //decrement the iterator
                                if(str_start == -1) str_start = dictionary.capacity-1; //move the iterator to the other end of the ring
                                str_end = str_start;
                            }
                            ImGui::PopTextWrapPos();
                        }
                    }ImGui::EndChild();
                    ImGui::PopStyleColor();
                    
                    //// command text input ////
                    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
                    if(show_autocomplete) input_text_flags = ImGuiInputTextFlags_None;
                    
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorToVec4(color::VERY_DARK_CYAN));
                    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 16);
                    //TODO(delle,OpCl) this can be optimized by reducing the amount of string copies
                    if(ImGui::InputText("##console_input_text", input_buffer, input_max_size-1, input_text_flags, &TextEditCallback, 0)) {
                        //add input to history
                        string input = input_buffer;
                        Log("^c=cyan^/^c^^c=dcyan^\\^c^ " + input + "\n");
                        scroll_to_bottom = true;
                        
                        //send input to command system
                        if(input.size){
                            //!Incomplete
                            //add input to input_history if not already in it
                            bool already_in_history = false;
                            for(int i = 0; i < input_history_length; ++i){
                                if(strcmp(input_buffer, input_history+(input_max_size*i)) == 0){
                                    already_in_history = true;
                                    break;
                                }
                            }
                            if(!already_in_history){
                                memcpy(input_history+(input_max_size*input_history_index), input_buffer, input_max_size);
                                input_history_index = (input_history_index + 1) % (input_history_length - 1);
                            }
                            
                            Cmd::Run(input);
                            memset(input_buffer, 0, input_max_size);
                        }
                        ImGui::SetKeyboardFocusHere(-1);
                    }
                    ImGui::SetItemDefaultFocus();
                    ImGui::PopStyleColor();
                    
                }ImGui::End();
                ImGui::PopStyleVar(1);
                ImGui::PopStyleColor(7);
            }
        }
    }
};