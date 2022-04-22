/*Index:
@vars
@add
@run
@init
*/

#include "memory.h"

#define DESHI_CMD_START(name, desc) \
deshi__last_cmd_desc = str8_lit(desc); \
auto deshi__cmd__##name = [](str8* args, u32 arg_count) -> void

#define DESHI_CMD_END_NO_ARGS(name) \
; \
cmd_add(deshi__cmd__##name, str8_lit(#name), deshi__last_cmd_desc, 0, 0)

#define DESHI_CMD_END(name, ...) \
; \
local Type deshi__cmd__##name##args[] = {__VA_ARGS__}; \
cmd_add(deshi__cmd__##name, str8_lit(#name), deshi__last_cmd_desc, deshi__cmd__##name##args, ArrayCount(deshi__cmd__##name##args))

//TODO remove the need for this by having functions take in str8
#define temp_str8_cstr(s) (const char*)str8_copy(s, deshi_temp_allocator).str

//-////////////////////////////////////////////////////////////////////////////////////////////////
//@vars
local array<Command> deshi__cmd_commands(deshi_allocator);
local array<Alias> deshi__cmd_aliases(deshi_allocator);
local str8 deshi__last_cmd_desc;

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
	
	str8_builder builder;
	str8_builder_init(&builder, name, deshi_allocator);
	forI(arg_count){
		cmd->max_args++;
		str8_builder_append(&builder, str8_lit(" "));
		if(args[i] & CmdArgument_OPTIONAL){
			str8_builder_append(&builder, str8_lit("["));
		}else{
			str8_builder_append(&builder, str8_lit("<"));
			cmd->min_args++;
		}
		
		if      (args[i] & CmdArgument_S32){
			str8_builder_append(&builder, str8_lit("S32"));
		}else if(args[i] & CmdArgument_String){
			str8_builder_append(&builder, str8_lit("String"));
		}else{
			Assert(!"unhandled command arguent");
			NotImplemented;
		}
		
		if(args[i] & CmdArgument_OPTIONAL){
			str8_builder_append(&builder, str8_lit("]"));
		}else{
			str8_builder_append(&builder, str8_lit(">"));
		}
	}
	str8_builder_fit(&builder);
	cmd->usage.str   = builder.str;
	cmd->usage.count = builder.count;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//@run
void cmd_run(str8 input){
	array<str8> args(deshi_temp_allocator);
	
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
						LogE("cmd", "Command '",temp_str8_cstr(args[0]),"' requires at least ",it->min_args," arguments");
					}else if(args_count > it->max_args){
						LogE("cmd", "Command '",temp_str8_cstr(args[0]),"' requires at most  ",it->max_args," arguments");
					}else{
						it->func(args.data+1, args_count);
					}
				}else{
					LogE("cmd", "Command '",temp_str8_cstr(args[0]),"' has no registered function");
				}
				found = true;
				break;
			}
		}
		if(!found){
			LogE("cmd", "Unknown command '",temp_str8_cstr(args[0]),"'");
		}
	}
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//@init
void cmd_init(){
	AssertDS(DS_MEMORY, "Attempt to load Cmd without loading Memory first");
	deshiStage |= DS_CMD;
	TIMER_START(t_s);
	
	DESHI_CMD_START(test, "testing sandbox"){
		console_log("{{c=magen}blah blah");
	}DESHI_CMD_END_NO_ARGS(test);
	
	DESHI_CMD_START(dir, "List the contents of a directory"){
		array<File> files = get_directory_files(temp_str8_cstr(args[0]));
		char time_str[1024];
		if(files.count){
			Logf("cmd","Directory of '%s':",args[0].str);
			forE(files){
				strftime(time_str,1024,"%D  %R",localtime((time_t*)&it->time_last_write));
				Logf("cmd","%s    %s  %-30s  %lu bytes", time_str,((it->is_directory)?"<DIR> ":"<FILE>"),
					 it->name,it->bytes_size);
			}
		}
	}DESHI_CMD_END(dir, CmdArgument_String);
	
	DESHI_CMD_START(rm, "Remove a file"){
		delete_file(temp_str8_cstr(args[0]));
	}DESHI_CMD_END(rm, CmdArgument_String);
	
	DESHI_CMD_START(file_exists, "Checks if a file exists"){
		Log("cmd","File '",temp_str8_cstr(args[0]),"' ",(file_exists(temp_str8_cstr(args[0]))) ? "exists." : "does not exist.");
	}DESHI_CMD_END(file_exists, CmdArgument_String);
	
	DESHI_CMD_START(rename, "Renames a file"){
		rename_file(temp_str8_cstr(args[0]), temp_str8_cstr(args[1]));
	}DESHI_CMD_END(rename, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(add, "Adds two numbers together"){
		//TODO rework this to be 'calc' instead of just 'add'
		s32 i0 = atoi(temp_str8_cstr(args[0]));
		s32 i1 = atoi(temp_str8_cstr(args[1]));
		Log("cmd", i0," + ",i1," = ", i0+i1);
	}DESHI_CMD_END(add, CmdArgument_S32, CmdArgument_S32);
	
	DESHI_CMD_START(daytime, "Logs the time in day-time format"){
		Log("cmd", DeshTime->FormatDateTime("{w} {M}/{d}/{y} {h}:{m}:{s}").c_str());
	}DESHI_CMD_END_NO_ARGS(daytime);
	
	DESHI_CMD_START(time_engine, "Logs deshi engine times"){
		Log("cmd", DeshTime->FormatTickTime("Time:   {t}ms Window:{w}ms Input:{i}ms Admin:{a}ms\n"
											"Console:{c}ms Render:{r}ms Frame:{f}ms Delta:{d}ms").c_str());
	}DESHI_CMD_END_NO_ARGS(time_engine);
	
	DESHI_CMD_START(list, "Lists available commands"){
		str8_builder builder;
		str8_builder_init(&builder, {}, deshi_temp_allocator);
		forE(deshi__cmd_commands){
			str8_builder_append(&builder, it->name);
			str8_builder_append(&builder, str8_lit(": "));
			str8_builder_append(&builder, it->desc);
			str8_builder_append(&builder, str8_lit("\n"));
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
			if(!found) LogE("cmd", "Command '",temp_str8_cstr(args[0]),"' not found");
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
				DeshWindow->UpdateDisplayMode(DisplayMode_Windowed);
				Log("cmd", "Window display mode updated to 'windowed'");
			}break;
			case 1:{
				DeshWindow->UpdateDisplayMode(DisplayMode_Borderless);
				Log("cmd", "Window display mode updated to 'borderless'");
			}break;
			case 2:{
				DeshWindow->UpdateDisplayMode(DisplayMode_Fullscreen);
				Log("cmd", "Window display mode updated to 'fullscreen'");
			}break;
			default:{
				Log("cmd", "Display Modes: 0=Windowed, 1=Borderless, 2=Fullscreen");
			}break;
		}
	}DESHI_CMD_END(window_display_mode, CmdArgument_S32);
	
	DESHI_CMD_START(window_cursor_mode, "Changes whether the cursor is in default(0), first person(1), or hidden(2) mode"){
		s32 mode = atoi((const char*)args[0].str);
		switch(mode){
			case 0:{
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				Log("cmd", "Cursor mode updated to 'default'");
			}break;
			case 1:{
				DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);
				Log("cmd", "Cursor mode updated to 'first person'");
			}break;
			case 2:{
				DeshWindow->UpdateCursorMode(CursorMode_Hidden);
				Log("cmd", "Cursor mode updated to 'hidden'");
			}break;
			default:{
				Log("cmd", "Cursor Modes: 0=Default, 1=First Person, 2=Hidden");
			}break;
		}
	}DESHI_CMD_END(window_cursor_mode, CmdArgument_S32);
	
	DESHI_CMD_START(window_raw_input, "Changes whether the window uses raw input"){
		s32 mode = atoi((const char*)args[0].str);
		switch(mode){
			case 0:{
				DeshWindow->UpdateRawInput(false);
				Log("cmd", "Raw input updated to 'false'");
			}break;
			case 1:{
				DeshWindow->UpdateRawInput(true);
				Log("cmd", "Raw input updated to 'true'");
			}break;
			default:{
				Log("cmd", "Raw Input: 0=False, 1=True");
			}break;
		}
	}DESHI_CMD_END(window_raw_input, CmdArgument_S32);
	
	DESHI_CMD_START(window_resizable, "Changes whether the window is resizable"){
		u32 mode = atoi((const char*)args[0].str);
		switch(mode){
			case 0:{
				DeshWindow->UpdateResizable(false);
				Log("cmd", "Window resizability updated to 'false'");
			}break;
			case 1:{
				DeshWindow->UpdateResizable(true);
				Log("cmd", "Window resizability updated to 'true'");
			}break;
			default:{
				Log("cmd", "Window resizability: 0=False, 1=True");
			}break;
		}
	}DESHI_CMD_END(window_resizable, CmdArgument_S32);
	
	DESHI_CMD_START(window_info, "Lists window's vars"){
		str8 dispMode;
		switch(DeshWindow->displayMode){
			case(DisplayMode_Windowed):  { dispMode = str8_lit("Windowed"); }break;
			case(DisplayMode_Borderless):{ dispMode = str8_lit("Borderless Windowed"); }break;
			case(DisplayMode_Fullscreen):{ dispMode = str8_lit("Fullscreen"); }break;
		}
		str8 cursMode;
		switch(DeshWindow->cursorMode){
			case(CursorMode_Default):    { cursMode = str8_lit("Default"); }break;
			case(CursorMode_FirstPerson):{ cursMode = str8_lit("First Person"); }break;
			case(CursorMode_Hidden):     { cursMode = str8_lit("Hidden"); }break;
		}
		Log("cmd",
			"Window Info"
			"\n    Window Position: ",DeshWindow->x,",",DeshWindow->y,
			"\n    Window Dimensions: ",DeshWindow->width,"x",DeshWindow->height,
			"\n    Screen Dimensions: ",DeshWindow->screenWidth,"x",DeshWindow->screenHeight,
			"\n    Refresh Rate: ",DeshWindow->refreshRate,
			"\n    Screen Refresh Rate: ",DeshWindow->screenRefreshRate,
			"\n    Display Mode: ",(const char*)dispMode.str,
			"\n    Cursor Mode: ",(const char*)cursMode.str,
			"\n    Raw Input: ",DeshWindow->rawInput,
			"\n    Resizable: ",DeshWindow->resizable,
			"\n    Restores: ",DeshWindow->restoreX,",",DeshWindow->restoreY,
			" ",DeshWindow->restoreW,"x",DeshWindow->restoreH);
	}DESHI_CMD_END_NO_ARGS(window_info);
	
	DESHI_CMD_START(mat_list, "Lists the materials and their info"){
		Log("cmd", "Material List:\nName\tShader\tTextures");
		forI(Storage::MaterialCount()){
			Material* mat = Storage::MaterialAt(i);
			str8_builder builder;
			str8_builder_init(&builder, str8{(u8*)mat->name, (s64)strlen(mat->name)}, deshi_temp_allocator);
			str8_builder_append(&builder, str8_lit("\t"));
			str8_builder_append(&builder, str8{(u8*)ShaderStrings[mat->shader], (s64)strlen(ShaderStrings[mat->shader])});
			str8_builder_append(&builder, str8_lit("\t"));
			forI(mat->textures.count){
				str8_builder_append(&builder, str8_lit(" "));
				str8_builder_append(&builder, str8{(u8*)Storage::TextureName(mat->textures[i]), (s64)strlen(Storage::TextureName(mat->textures[i]))});
			}
			Log("cmd", (const char*)builder.str);
		}
	}DESHI_CMD_END_NO_ARGS(mat_list);
	
	DESHI_CMD_START(mat_texture, "Changes a texture of a material"){
		s32 matID = atoi(temp_str8_cstr(args[0]));
		s32 texSlot = atoi(temp_str8_cstr(args[1]));
		s32 texID = atoi(temp_str8_cstr(args[2]));
		Storage::MaterialAt(matID)->textures[texSlot] = texID;
		Log("cmd", "Updated material ",Storage::MaterialName(matID),"'s texture",texSlot," to ",Storage::TextureName(texID));
	}DESHI_CMD_END(mat_texture, CmdArgument_S32, CmdArgument_S32, CmdArgument_S32);
	
	DESHI_CMD_START(mat_shader, "Changes the shader of a material"){
		s32 matID = atoi(temp_str8_cstr(args[0]));
		s32 shader = atoi(temp_str8_cstr(args[1]));
		Storage::MaterialAt(matID)->shader = (Shader)shader;
		Log("cmd", "Updated material ",Storage::MaterialName(matID),"'s shader to ", ShaderStrings[shader]);
	}DESHI_CMD_END(mat_shader, CmdArgument_S32, CmdArgument_S32);
	
	DESHI_CMD_START(shader_reload, "Reloads specified shader"){
		s32 id = atoi((const char*)args[0].str);
		if(id == -1){
			Render::ReloadAllShaders();
			console_log("{{t=CMD,c=magen}Reloaded all shaders");
		}else if(id < Shader_COUNT){
			Render::ReloadShader(id);
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
		TIMER_START(t_l);
		TextureType type = TextureType_2D;
		if(arg_count == 2) type = (TextureType)atoi(temp_str8_cstr(args[1]));
		Storage::CreateTextureFromFile(temp_str8_cstr(args[0]), ImageFormat_RGBA, type);
		Log("cmd", "Loaded texture '",temp_str8_cstr(args[0]),"' in ",TIMER_END(t_l),"ms");
	}DESHI_CMD_END(texture_load, CmdArgument_String, CmdArgument_S32|CmdArgument_OPTIONAL);
	
	DESHI_CMD_START(texture_list, "Lists the textures and their info"){
		Log("cmd", "Texture List:\nName\tWidth\tHeight\tDepth\tMipmaps\tType");
		forI(Storage::TextureCount()){
			Texture* tex = Storage::TextureAt(i);
			Log("cmd", '\n',tex->name,'\t',tex->width,'\t',tex->height,'\t',tex->depth, '\t',tex->mipmaps,'\t',TextureTypeStrings[tex->type]);
		}
	}DESHI_CMD_END_NO_ARGS(texture_list);
	
	DESHI_CMD_START(quit, "Exits the application"){
		DeshWindow->Close();
	}DESHI_CMD_END_NO_ARGS(quit);
	
	
	LogS("deshi","Finished commands initialization in ",TIMER_END(t_s),"ms");
}

#undef DESHI_CMD_START
#undef DESHI_CMD_END