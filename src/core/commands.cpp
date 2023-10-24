/*Index:
@vars
@utils
@add
@run
@init
*/


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@vars
local arrayT<Command> deshi__cmd_commands(deshi_allocator);
local arrayT<Alias> deshi__cmd_aliases(deshi_allocator);
local str8 deshi__last_cmd_desc;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@utils
#define DESHI_CMD_START(name, desc)      \
  deshi__last_cmd_desc = str8_lit(desc); \
  auto deshi__cmd__##name = [](str8* args, u32 arg_count) -> void

#define DESHI_CMD_END_NO_ARGS(name) \
  ;                                 \
  cmd_add(deshi__cmd__##name, str8_lit(#name), deshi__last_cmd_desc, 0, 0)

#define DESHI_CMD_END(name, ...)                         \
  ;                                                      \
  local Type deshi__cmd__##name##args[] = {__VA_ARGS__}; \
  cmd_add(deshi__cmd__##name, str8_lit(#name), deshi__last_cmd_desc, deshi__cmd__##name##args, ArrayCount(deshi__cmd__##name##args))

//TODO remove the need for this by having functions take in str8
#define temp_str8_cstr(s) (const char*)str8_copy(s, deshi_temp_allocator).str


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@add
void cmd_add(CmdFunc func, str8 name, str8 desc, Type* args, u32 arg_count){
	deshi__cmd_commands.add(Command{});
	Command* cmd = &deshi__cmd_commands[deshi__cmd_commands.count-1];
	cmd->func      = func;
	cmd->name      = name;
	cmd->desc      = desc;
	cmd->args      = args;
	cmd->arg_count = arg_count;
	cmd->min_args  = 0;
	cmd->max_args  = 0;
	
	dstr8 builder;
	dstr8_init(&builder, name, deshi_allocator);
	forI(arg_count){
		cmd->max_args++;
		dstr8_append(&builder, str8_lit(" "));
		if(args[i] & CmdArgument_OPTIONAL){
			dstr8_append(&builder, str8_lit("["));
		}else{
			dstr8_append(&builder, str8_lit("<"));
			cmd->min_args++;
		}
		
		if      (args[i] & CmdArgument_S32){
			dstr8_append(&builder, str8_lit("S32"));
		}else if(args[i] & CmdArgument_String){
			dstr8_append(&builder, str8_lit("String"));
		}else{
			Assert(!"unhandled command arguent");
			NotImplemented;
		}
		
		if(args[i] & CmdArgument_OPTIONAL){
			dstr8_append(&builder, str8_lit("]"));
		}else{
			dstr8_append(&builder, str8_lit(">"));
		}
	}
	dstr8_fit(&builder);
	cmd->usage.str   = builder.str;
	cmd->usage.count = builder.count;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@run
void cmd_run(str8 input){
	arrayT<str8> args(deshi_temp_allocator);
	
	//split input by spaces (treating double quoted strings as one item)
	//TODO nested aliases
	while(input){
		str8_advance_while(&input, ' ');
		if(!input) break;
		
		str8 word = input;
		if(str8_index(word, 0).codepoint == '\"'){
			str8_advance(&word);
			word = str8_eat_until(word, '\"');
			if(word){
				args.add(word);
				input.str    = word.str+word.count+1;
				input.count -= word.count+2;
			}else{
				args.add(input);
				break;
			}
		}else{
			word = str8_eat_until(word, ' ');
			if(word){
				b32 aliased = false;
				forE(deshi__cmd_aliases){
					if(str8_equal(word, it->alias)){
						str8 temp = it->actual;
						while(temp){
							str8_advance_while(&temp, ' ');
							str8 before = temp;
							str8_advance_until(&temp, ' ');
							args.add(str8{before.str, before.count-temp.count});
						}
						aliased = true;
						break;
					}
				}
				if(!aliased) args.add(word);
				
				input.str    = word.str+word.count;
				input.count -= word.count;
			}else{
				b32 aliased = false;
				forE(deshi__cmd_aliases){
					if(str8_equal(input, it->alias)){
						str8 temp = it->actual;
						while(temp){
							str8_advance_while(&temp, ' ');
							str8 before = temp;
							str8_advance_until(&temp, ' ');
							args.add(str8{before.str, before.count-temp.count});
						}
						aliased = true;
						break;
					}
				}
				if(!aliased) args.add(input);
				break;
			}
		}
	}
	
	if(args.count){
		u32 args_count = args.count-1;
		b32 found = false;
		forE(deshi__cmd_commands){
			if(str8_equal(args[0], it->name)){
				if(it->func){
					if(args_count < it->min_args){
						LogE("cmd", "Command '",args[0],"' requires at least ",it->min_args," arguments");
					}else if(args_count > it->max_args){
						LogE("cmd", "Command '",args[0],"' requires at most  ",it->max_args," arguments");
					}else{
						it->func(args.data+1, args_count);
					}
				}else{
					LogE("cmd", "Command '",args[0],"' has no registered function");
				}
				found = true;
				break;
			}
		}
		if(!found){
			LogE("cmd", "Unknown command '",args[0],"'");
		}
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@init
void cmd_init(){
	DeshiStageInitStart(DS_CMD, DS_MEMORY, "Attempted to initialize Cmd module before initializing Memory module");
	
	DESHI_CMD_START(dir, "List the contents of a directory"){
		FixMe;
		arrayT<File> files = {};file_search_directory(args[0]);
		char time_str[1024];
		if(files.count){
			Log("cmd","Directory of '",args[0],"':");
			forE(files){
				strftime(time_str,1024,"%D  %R",localtime((time_t*)&it->last_write_time));
				Logf("cmd","%s    %s  %-30s  %lu bytes", time_str,((it->type == FileType_Directory)?"<DIR> ":"<FILE>"),
					 (const char*)it->name.str,it->bytes);
			}
		}
	}DESHI_CMD_END(dir, CmdArgument_String);
	
	DESHI_CMD_START(rm, "Remove a file"){
		file_delete(args[0], FileDeleteFlags_File);
	}DESHI_CMD_END(rm, CmdArgument_String);
	
	DESHI_CMD_START(cp, "Copies a file/directory to a new location"){
		file_copy(args[0], args[1]);
	}DESHI_CMD_END(cp, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(file_exists, "Checks if a file exists"){
		Log("cmd","File '",args[0],"' ",(file_exists(args[0])) ? "exists." : "does not exist.");
	}DESHI_CMD_END(file_exists, CmdArgument_String);
	
	DESHI_CMD_START(rename, "Renames a file"){
		file_rename(args[0], args[1]);
	}DESHI_CMD_END(rename, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(add, "Adds two numbers together"){
		//TODO rework this to be 'calc' instead of just 'add'
		s32 i0 = atoi(temp_str8_cstr(args[0]));
		s32 i1 = atoi(temp_str8_cstr(args[1]));
		Log("cmd", i0," + ",i1," = ", i0+i1);
	}DESHI_CMD_END(add, CmdArgument_S32, CmdArgument_S32);
	
	DESHI_CMD_START(daytime, "Logs the time in day-time format"){
		u8 time_buffer[512];
		time_t rawtime = time(0);
		strftime((char*)time_buffer, 512, "%c", localtime(&rawtime));
		Log("cmd",(const char*)time_buffer);
	}DESHI_CMD_END_NO_ARGS(daytime);
	
	DESHI_CMD_START(list, "Lists available commands"){
		dstr8 builder;
		dstr8_init(&builder, {}, deshi_temp_allocator);
		forE(deshi__cmd_commands){
			dstr8_append(&builder, it->name);
			dstr8_append(&builder, str8_lit(": "));
			dstr8_append(&builder, it->desc);
			dstr8_append(&builder, str8_lit("\n"));
		}
		Log("cmd", (const char*)builder.str);
	}DESHI_CMD_END_NO_ARGS(list);
	
	DESHI_CMD_START(help, "Logs description and usage of specified command"){
		if(arg_count){
			b32 found = false;
			forE(deshi__cmd_commands){
				if(str8_equal(it->name, args[0])){
					Log("cmd", (const char*)it->desc.str);
					Log("cmd", (const char*)it->usage.str);
					found = true;
					break;
				}
			}
			if(!found) LogE("cmd", "Command '",args[0],"' not found");
		}else{
			Log("cmd", "Use 'help <command>' to get a description and usage of the command");
			Log("cmd", "Use 'list' to get a list of all available commands");
			Log("cmd", "Usage Format: command <required> [optional]");
		}
	}DESHI_CMD_END(help, CmdArgument_String|CmdArgument_OPTIONAL);
	
	DESHI_CMD_START(alias, "Gives an alias to specified command and arguments"){
		//check that alias' actual won't contain the alias
		str8 cursor = args[1];
		u32 alias_len = str8_length(args[0]);
		u32 prev_codepoint = -1;
		while(cursor){
			if(args[0].count > cursor.count) break;
			if(   str8_nequal(cursor, args[0], alias_len)
			   && (prev_codepoint == -1 || prev_codepoint == ' ')
			   && (cursor.str+args[0].count == cursor.str+cursor.count || str8_index(cursor, alias_len).codepoint == ' ')){
				LogE("cmd", "Aliases can't be recursive");
				return;
			}
			prev_codepoint = str8_advance(&cursor).codepoint;
		}
		
		//check that alias doesnt start with a number
		if(isdigit(str8_index(args[0], 0).codepoint)){
			LogE("cmd", "Aliases can't start with a number");
			return;
		}
		
		//check if name is used by a command
		forE(deshi__cmd_commands){
			if(str8_equal(it->name, args[0])){
				LogE("cmd", "Aliases can't use the same name as an existing command");
				return;
			}
		}
		
		//check if alias already exists
		u32 idx = -1;
		forE(deshi__cmd_aliases){
			if(str8_equal(it->alias, args[0])){
				idx = it-it_begin;
				break;
			}
		}
		if(idx == -1){
			deshi__cmd_aliases.add({str8_copy(args[0], deshi_allocator), str8_copy(args[1], deshi_allocator)});
		}else{
			memory_zfree(args[1].str);
			deshi__cmd_aliases[idx].actual = str8_copy(args[1], deshi_allocator);
		}
	}DESHI_CMD_END(alias, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(aliases, "Lists available aliases"){
		forE(deshi__cmd_aliases){
			Log("cmd", (const char*)it->alias.str,": ",(const char*)it->actual.str);
		}
	}DESHI_CMD_END_NO_ARGS(aliases);
	
	DESHI_CMD_START(window_display_mode, "Changes whether the window is in windowed(0), borderless(1), or fullscreen(2) mode"){
		s32 mode = atoi((const char*)args[0].str);
		switch(mode){
			case 0:{
				window_display_mode(window_active, DisplayMode_Windowed);
				Log("cmd", "Window display mode updated to 'windowed'");
			}break;
			case 1:{
				window_display_mode(window_active, DisplayMode_Borderless);
				Log("cmd", "Window display mode updated to 'borderless'");
			}break;
			case 2:{
				window_display_mode(window_active, DisplayMode_BorderlessMaximized);
				Log("cmd", "Window display mode updated to 'borderless maximized'");
			}break;
			case 3:{
				window_display_mode(window_active, DisplayMode_Fullscreen);
				Log("cmd", "Window display mode updated to 'fullscreen'");
			}break;
			default:{
				Log("cmd", "Display Modes: 0=Windowed, 1=Borderless, 2=Borderless Maximized, 3=Fullscreen");
			}break;
		}
	}DESHI_CMD_END(window_display_mode, CmdArgument_S32);
	
	DESHI_CMD_START(window_set_title, "Changes the title of the active window"){
		window_set_title(window_active, args[0]);
		Log("cmd","Updated active window's title to: ",args[0]);
	}DESHI_CMD_END(window_set_title, CmdArgument_String);
	
	DESHI_CMD_START(window_set_cursor_mode, "Changes whether the cursor is in default(0), first person(1), or hidden(2) mode"){
		s32 mode = atoi((const char*)args[0].str);
		switch(mode){
			case 0:{
				window_set_cursor_mode(window_active, CursorMode_Default);
				Log("cmd", "Cursor mode updated to 'default'");
			}break;
			case 1:{
				window_set_cursor_mode(window_active, CursorMode_FirstPerson);
				Log("cmd", "Cursor mode updated to 'first person'");
			}break;
			default:{
				Log("cmd", "Cursor Modes: 0=Default, 1=First Person");
			}break;
		}
	}DESHI_CMD_END(window_set_cursor_mode, CmdArgument_S32);
	
	DESHI_CMD_START(window_raw_input, "Changes whether the window uses raw input"){
		LogE("cmd","Raw Input not setup yet");
		return;
		
		s32 mode = atoi((const char*)args[0].str);
		switch(mode){
			case 0:{
				//window_active->UpdateRawInput(false);
				Log("cmd", "Raw input updated to 'false'");
			}break;
			case 1:{
				//window_active->UpdateRawInput(true);
				Log("cmd", "Raw input updated to 'true'");
			}break;
			default:{
				Log("cmd", "Raw Input: 0=False, 1=True");
			}break;
		}
	}DESHI_CMD_END(window_raw_input, CmdArgument_S32);
	
	DESHI_CMD_START(window_info, "Lists window's vars"){
		str8 dispMode;
		switch(window_active->display_mode){
			case(DisplayMode_Windowed):           { dispMode = str8_lit("Windowed"); }break;
			case(DisplayMode_Borderless):         { dispMode = str8_lit("Borderless"); }break;
			case(DisplayMode_BorderlessMaximized):{ dispMode = str8_lit("Borderless Maximized"); }break;
			case(DisplayMode_Fullscreen):         { dispMode = str8_lit("Fullscreen"); }break;
		}
		str8 cursMode;
		switch(window_active->cursor_mode){
			case(CursorMode_Default):    { cursMode = str8_lit("Default"); }break;
			case(CursorMode_FirstPerson):{ cursMode = str8_lit("First Person"); }break;
		}
		Log("cmd",
			"Window Info"
			"\n    Index:          ",window_active->index,
			"\n    Title:          ",window_active->title,
			"\n    Client Pos:     ",window_active->x,",",window_active->y,
			"\n    Client Dims:    ",window_active->width,"x",window_active->height,
			"\n    Decorated Pos:  ",window_active->position_decorated.x,",",window_active->position_decorated.y,
			"\n    Decorated Dims: ",window_active->dimensions_decorated.x,"x",window_active->dimensions_decorated.y,
			"\n    Display Mode:   ",dispMode,
			"\n    Cursor Mode:    ",cursMode,
			"\n    Restore Pos:    ",window_active->restore.x,",",window_active->restore.y,
			"\n    Restore Dims:   ",window_active->restore.width,"x",window_active->restore.height);
	}DESHI_CMD_END_NO_ARGS(window_info);
	
	DESHI_CMD_START(mat_list, "Lists the materials and their info"){
		Log("cmd", "Material List:\nName\tShader\tTextures");
		forI(arrlenu(assets_material_array())){
			Material* mat = assets_material_array()[i];
			dstr8 builder;
			dstr8_init(&builder, str8{(u8*)mat->name, (s64)strlen(mat->name)}, deshi_temp_allocator);
			dstr8_append(&builder, str8_lit("\t"));
			dstr8_append(&builder, ShaderStrings[mat->shader]);
			dstr8_append(&builder, str8_lit("\t"));
			if(mat->texture_array){
				for_stb_array(mat->texture_array){
					dstr8_append(&builder, str8_lit(" "));
					dstr8_append(&builder, str8_from_cstr((*it)->name));
				}
			}
			Log("cmd", (const char*)builder.str);
		}
	}DESHI_CMD_END_NO_ARGS(mat_list);
	
	DESHI_CMD_START(mat_texture, "Changes a texture of a material"){
		Material* mat = 0;
		const char* mat_name = temp_str8_cstr(args[0]);
		for_stb_array(assets_material_array()){
			if(strcmp((*it)->name, mat_name) == 0){
				mat = *it;
				break;
			}
		}
		if(mat == 0){
			LogE("cmd","Failed to update material texture. There is no material named '",args[0],"'.");
			return;
		}
		
		s32 texSlot = atoi(temp_str8_cstr(args[1]));
		if(mat->texture_array == 0){
			LogE("cmd","Failed to update material texture. The material '",args[0],"' has no textures.");
			return;
		}
		if(texSlot < 0 || texSlot >= arrlen(mat->texture_array)){
			LogE("cmd","Failed to update material texture. The supplied texture index '",texSlot,"' is outside of bounds '0..",arrlen(mat->texture_array),"'.");
			return;
		}
		
		Texture* tex = 0;
		const char* tex_name = temp_str8_cstr(args[2]);
		for_stb_array(assets_texture_array()){
			if(strcmp((*it)->name, tex_name) == 0){
				tex = *it;
				break;
			}
		}
		if(tex == 0){
			LogE("cmd","Failed to update material texture. There is no texture named '",args[2],"'.");
			return;
		}
		
		mat->texture_array[texSlot] = tex;
		Log("cmd", "Updated material ",mat->name,"'s texture",texSlot," to ",tex->name);
	}DESHI_CMD_END(mat_texture, CmdArgument_String, CmdArgument_S32, CmdArgument_String);
	
	DESHI_CMD_START(mat_shader, "Changes the shader of a material"){
		Material* mat = 0;
		const char* mat_name = temp_str8_cstr(args[0]);
		for_stb_array(assets_material_array()){
			if(strcmp((*it)->name, mat_name) == 0){
				mat = *it;
				break;
			}
		}
		if(mat == 0){
			LogE("cmd","Failed to update material shader. There is no material named '",args[0],"'.");
			return;
		}
		
		s32 shader = atoi(temp_str8_cstr(args[1]));
		if(shader < 0 || shader >= Shader_COUNT){
			LogE("cmd","Failed to update material shader. There is no shader with value '",shader,"'.");
			return;
		}
		
		mat->shader = (Shader)shader;
		Log("cmd", "Updated material ",mat->name,"'s shader to ",ShaderStrings[shader]);
	}DESHI_CMD_END(mat_shader, CmdArgument_S32, CmdArgument_S32);
	
	DESHI_CMD_START(shader_reload, "Reloads specified shader"){
		s32 id = atoi(temp_str8_cstr(args[0]));
		if(id == -1){
			render_reload_all_shaders();
			console_log("{{t=CMD,c=magen}Reloaded all shaders");
		}else if(id < Shader_COUNT){
			render_reload_shader(id);
			console_log("{{t=CMD,c=magen}Reloaded '",ShaderStrings[id]);
		}else{
			LogE("cmd", "There is no shader with id: ",id);
		}
	}DESHI_CMD_END(shader_reload, CmdArgument_S32);
	
	DESHI_CMD_START(shader_list, "Lists the shaders and their info"){
		Log("cmd", "Shader List:\nID\tName");
		forI(ArrayCount(ShaderStrings)){
			Log("cmd", i,'\t',ShaderStrings[i]);
		}
	}DESHI_CMD_END_NO_ARGS(shader_list);
	
	DESHI_CMD_START(texture_load, "Loads a specific texture"){
		Stopwatch load_stopwatch = start_stopwatch();
		assets_texture_create_from_file_simple(args[0]);
		Log("cmd", "Loaded texture '",args[0],"' in ",peek_stopwatch(load_stopwatch),"ms");
	}DESHI_CMD_END(texture_load, CmdArgument_String);
	
	DESHI_CMD_START(texture_list, "Lists the textures and their info"){
		Log("cmd", "Texture List:\nName\tWidth\tHeight\tDepth\tMipmaps\tType");
		for_stb_array(assets_texture_array()){
			Texture* tex = *it;
			Log("cmd", '\n',tex->name,'\t',tex->width,'\t',tex->height,'\t',tex->depth, '\t',tex->mipmaps,'\t',TextureTypeStrings[tex->type]);
		}
	}DESHI_CMD_END_NO_ARGS(texture_list);
	
	DESHI_CMD_START(show_ui_metrics, "Toggles the visibility of the UI metrics window//TODO(sushi) switch to ui2 metrics"){
		//TODO(sushi) switch to ui2 metrics 
	}DESHI_CMD_END_NO_ARGS(show_ui_metrics);
	
	DESHI_CMD_START(show_ui_demo, "Toggles the visibility of the UI demo window"){
		ui_demo();
	}DESHI_CMD_END_NO_ARGS(show_ui_demo);
	
	DESHI_CMD_START(quit, "Exits the application"){
		platform_exit();
	}DESHI_CMD_END_NO_ARGS(quit);
	
	DeshiStageInitEnd(DS_CMD);
}

#undef DESHI_CMD_START
#undef DESHI_CMD_END_NO_ARGS
#undef DESHI_CMD_END
