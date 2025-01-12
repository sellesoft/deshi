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
	//split input by semicolons and execute each command
	while(input){
		str8_advance_while(&input, ' '); //skip leading whitespace
		if(!input) break;
		
		//find next semicolon that's not in quotes
		str8 command = input;
		str8 cursor = input;
		b32 in_quotes = false;
		while(cursor){
			if(str8_index(cursor, 0).codepoint == '\"'){
				in_quotes = !in_quotes;
			}else if(!in_quotes && str8_index(cursor, 0).codepoint == ';'){
				command.count = cursor.str - command.str;
				input.str = cursor.str + 1;
				input.count = input.count - command.count - 1;
				break;
			}
			str8_advance(&cursor);
		}
		if(!cursor){
			input = str8{}; //no more semicolons
		}
		
		//skip empty commands
		str8_advance_while(&command, ' ');
		if(!command) continue;
		
		//process single command
		str8* args = array_create(str8, 4, deshi_temp_allocator);
		
		//split command by spaces (treating double quoted strings as one item)
		str8 cmd_input = command;
		while(cmd_input){
			str8_advance_while(&cmd_input, ' ');
			if(!cmd_input) break;
			
			str8 word = cmd_input;
			if(str8_index(word, 0).codepoint == '\"'){
				str8_advance(&word);
				word = str8_eat_until(word, '\"');
				if(word){
					array_push_value(args, word);
					cmd_input.str    = word.str+word.count+1;
					cmd_input.count -= word.count+2;
				}else{
					array_push_value(args, cmd_input);
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
								array_push_value(args, (str8{before.str, before.count-temp.count}));
							}
							aliased = true;
							break;
						}
					}
					if(!aliased) array_push_value(args, word);
					
					cmd_input.str    = word.str+word.count;
					cmd_input.count -= word.count;
				}else{
					b32 aliased = false;
					forE(deshi__cmd_aliases){
						if(str8_equal(cmd_input, it->alias)){
							str8 temp = it->actual;
							while(temp){
								str8_advance_while(&temp, ' ');
								str8 before = temp;
								str8_advance_until(&temp, ' ');
								array_push_value(args, (str8{before.str, before.count-temp.count}));
							}
							aliased = true;
							break;
						}
					}
					if(!aliased) array_push_value(args, cmd_input);
					break;
				}
			}
		}
		
		//execute the command
		u32 args_count = array_count(args);
		if(args_count){
			args_count -= 1; //skip the command name
			b32 found = false;
			forE(deshi__cmd_commands){
				if(str8_equal(args[0], it->name)){
					if(it->func){
						if(args_count < it->min_args){
							LogW("cmd", "Command '",args[0],"' requires at least ",it->min_args," arguments");
						}else if(args_count > it->max_args){
							LogW("cmd", "Command '",args[0],"' requires at most ",it->max_args," arguments");
						}else{
							it->func(args+1, args_count);
						}
					}else{
						LogW("cmd", "Command '",args[0],"' has no registered function");
					}
					found = true;
					break;
				}
			}
			if(!found){
				LogW("cmd", "Unknown command '",args[0],"'");
			}
		}
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@init
void cmd_init(){
	DeshiStageInitStart(DS_CMD, DS_MEMORY, "Attempted to initialize Cmd module before initializing Memory module");
	
	DESHI_CMD_START(add, "Adds two numbers together"){
		//TODO rework this to be 'calc' instead of just 'add'
		s32 i0 = atoi(temp_str8_cstr(args[0]));
		s32 i1 = atoi(temp_str8_cstr(args[1]));
		Log("cmd", i0," + ",i1," = ", i0+i1);
	}DESHI_CMD_END(add, CmdArgument_S32, CmdArgument_S32);
	
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
		
		//expand any aliases in the second argument
		str8* expanded_args = array_create(str8, 4, deshi_temp_allocator);
		str8 cmd_input = args[1];
		while(cmd_input){
			str8_advance_while(&cmd_input, ' ');
			if(!cmd_input) break;
			
			str8 word = cmd_input;
			if(str8_index(word, 0).codepoint == '\"'){
				str8_advance(&word);
				word = str8_eat_until(word, '\"');
				if(word){
					array_push_value(expanded_args, word);
					cmd_input.str    = word.str+word.count+1;
					cmd_input.count -= word.count+2;
				}else{
					array_push_value(expanded_args, cmd_input);
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
								array_push_value(expanded_args, (str8{before.str, before.count-temp.count}));
							}
							aliased = true;
							break;
						}
					}
					if(!aliased) array_push_value(expanded_args, word);
					
					cmd_input.str    = word.str+word.count;
					cmd_input.count -= word.count;
				}else{
					b32 aliased = false;
					forE(deshi__cmd_aliases){
						if(str8_equal(cmd_input, it->alias)){
							str8 temp = it->actual;
							while(temp){
								str8_advance_while(&temp, ' ');
								str8 before = temp;
								str8_advance_until(&temp, ' ');
								array_push_value(expanded_args, (str8{before.str, before.count-temp.count}));
							}
							aliased = true;
							break;
						}
					}
					if(!aliased) array_push_value(expanded_args, cmd_input);
					break;
				}
			}
		}
		
		//build expanded command string
		dstr8 expanded_cmd;
		dstr8_init(&expanded_cmd, {}, deshi_temp_allocator);
		forI(array_count(expanded_args)){
			if(i > 0) dstr8_append(&expanded_cmd, str8_lit(" "));
			dstr8_append(&expanded_cmd, expanded_args[i]);
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
			deshi__cmd_aliases.add({str8_copy(args[0], deshi_allocator), str8_copy(dstr8_peek(&expanded_cmd), deshi_allocator)});
		}else{
			memory_zfree(deshi__cmd_aliases[idx].actual.str);
			deshi__cmd_aliases[idx].actual = str8_copy(dstr8_peek(&expanded_cmd), deshi_allocator);
		}
	}DESHI_CMD_END(alias, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(aliases, "Lists available aliases"){
		forE(deshi__cmd_aliases){
			Log("cmd", (const char*)it->alias.str,": \"",(const char*)it->actual.str,"\"");
		}
	}DESHI_CMD_END_NO_ARGS(aliases);
	
	DESHI_CMD_START(cp, "Copies a file/directory to a new location"){
		file_copy(args[0], args[1]);
	}DESHI_CMD_END(cp, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(daytime, "Logs the time in day-time format"){
		u8 time_buffer[512];
		time_t rawtime = time(0);
		strftime((char*)time_buffer, 512, "%c", localtime(&rawtime));
		Log("cmd",(const char*)time_buffer);
	}DESHI_CMD_END_NO_ARGS(daytime);
	
	DESHI_CMD_START(device_info, "Logs information about the graphics, sound, and monitor devices"){
		MonitorInfo* monitors = platform_monitor_infos();
		
		dstr8 builder;
		dstr8_init(&builder, STR8("Device Info\n-----------------\n"), deshi_temp_allocator);
		
		dstr8_append(&builder, STR8("Monitors:\n"));
		if(monitors){
			for_array(monitors){
				dstr8_append(&builder, STR8("  "));
				dstr8_append(&builder, {(u8*)it->name, (s64)strlen((char*)it->name)});
				dstr8_append(&builder, STR8("\n"));
				dstr8_append(&builder, STR8("    Resolution: "));
				dstr8_append(&builder, it->resolution_x);
				dstr8_append(&builder, STR8("x"));
				dstr8_append(&builder, it->resolution_y);
				dstr8_append(&builder, STR8("\n"));
				dstr8_append(&builder, STR8("    Refresh Rate: "));
				dstr8_append(&builder, it->refresh_rate);
				dstr8_append(&builder, STR8("\n"));
			}
		}else{
			dstr8_append(&builder, STR8("  No monitors found\n"));
		}
		
		GraphicsDeviceInfo* graphics_devices = graphics_device_infos();
		if(graphics_devices){
			dstr8_append(&builder, STR8("Graphics Devices:\n"));
			for_array(graphics_devices){
				dstr8_append(&builder, STR8("  "));
				dstr8_append(&builder, (char*)it->device_name);
				dstr8_append(&builder, STR8("\n"));
				dstr8_append(&builder, STR8("    API: "));
				dstr8_append(&builder, (char*)it->api_name);
				dstr8_append(&builder, STR8(" "));
				dstr8_append(&builder, it->api_version_major);
				dstr8_append(&builder, STR8("."));
				dstr8_append(&builder, it->api_version_minor);
				dstr8_append(&builder, STR8("."));
				dstr8_append(&builder, it->api_version_patch);
				dstr8_append(&builder, STR8("\n"));
				dstr8_append(&builder, STR8("    Driver Version: "));
				dstr8_append(&builder, it->driver_version);
				dstr8_append(&builder, STR8("\n"));
			}
		}else{
			dstr8_append(&builder, STR8("  No graphics devices found\n"));
		}
		
		ProcessorInfo info = platform_processor_info();
		dstr8_append(&builder, STR8("Processor Info:\n"));
		dstr8_append(&builder, STR8("  "));
		dstr8_append(&builder, (char*)info.name);
		dstr8_append(&builder, STR8("\n"));
		if(info.type == 0){
			dstr8_append(&builder, STR8("    Vendor: "));
			if(info.x86.amd) dstr8_append(&builder, STR8("AMD"));
			if(info.x86.intel) dstr8_append(&builder, STR8("Intel"));
			dstr8_append(&builder, STR8("\n"));
			
			dstr8_append(&builder, STR8("    OS Support: "));
			if(info.x86.os_avx) dstr8_append(&builder, STR8("AVX "));
			if(info.x86.os_avx512) dstr8_append(&builder, STR8("AVX512 "));
			dstr8_append(&builder, STR8("\n"));
			
			dstr8_append(&builder, STR8("    SSE Support: "));
			if(info.x86.sse) dstr8_append(&builder, STR8("SSE "));
			if(info.x86.sse2) dstr8_append(&builder, STR8("SSE2 "));
			if(info.x86.sse3) dstr8_append(&builder, STR8("SSE3 "));
			if(info.x86.ssse3) dstr8_append(&builder, STR8("SSSE3 "));
			if(info.x86.sse41) dstr8_append(&builder, STR8("SSE4.1 "));
			if(info.x86.sse42) dstr8_append(&builder, STR8("SSE4.2 "));
			if(info.x86.sse4a) dstr8_append(&builder, STR8("SSE4a "));
			dstr8_append(&builder, STR8("\n"));
			
			dstr8_append(&builder, STR8("    AVX Support: "));
			if(info.x86.avx) dstr8_append(&builder, STR8("AVX "));
			if(info.x86.avx2) dstr8_append(&builder, STR8("AVX2 "));
			if(info.x86.avx512_f) dstr8_append(&builder, STR8("AVX512_F "));
			if(info.x86.avx512_cd) dstr8_append(&builder, STR8("AVX512_CD "));
			if(info.x86.avx512_pf) dstr8_append(&builder, STR8("AVX512_PF "));
			if(info.x86.avx512_er) dstr8_append(&builder, STR8("AVX512_ER "));
			if(info.x86.avx512_vl) dstr8_append(&builder, STR8("AVX512_VL "));
			if(info.x86.avx512_bw) dstr8_append(&builder, STR8("AVX512_BW "));
			if(info.x86.avx512_dq) dstr8_append(&builder, STR8("AVX512_DQ "));
			if(info.x86.avx512_ifma) dstr8_append(&builder, STR8("AVX512_IFMA "));
			if(info.x86.avx512_vbmi) dstr8_append(&builder, STR8("AVX512_VBMI "));
			if(info.x86.avx512_vpopcntdq) dstr8_append(&builder, STR8("AVX512_VPOCNTDQ "));
			if(info.x86.avx512_4vnniw) dstr8_append(&builder, STR8("AVX512_4VNNIW "));
			if(info.x86.avx512_4fmaps) dstr8_append(&builder, STR8("AVX512_4FMAPS "));
			if(info.x86.avx512_vnni) dstr8_append(&builder, STR8("AVX512_VNNI "));
			if(info.x86.avx512_bf16) dstr8_append(&builder, STR8("AVX512_BF16 "));
			if(info.x86.avx512_vmbi2) dstr8_append(&builder, STR8("AVX512_VMBI2 "));
			if(info.x86.avx512_vpclmul) dstr8_append(&builder, STR8("AVX512_VPCLMUL "));
			if(info.x86.avx512_bitalg) dstr8_append(&builder, STR8("AVX512_BITALG "));
			dstr8_append(&builder, STR8("\n"));
			
			dstr8_append(&builder, STR8("    Misc Support: "));
			if(info.x86.aes) dstr8_append(&builder, STR8("AES "));
			if(info.x86.sha) dstr8_append(&builder, STR8("SHA "));
			if(info.x86.gfni) dstr8_append(&builder, STR8("GFNI "));
			if(info.x86.vaes) dstr8_append(&builder, STR8("VAES "));
			dstr8_append(&builder, STR8("\n"));
		}
		
		dstr8_append(&builder, STR8("-----------------\n"));
		
		Log("cmd", dstr8_peek(&builder));
	}DESHI_CMD_END_NO_ARGS(device_info);
	
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
	
	DESHI_CMD_START(exec, "Executes commands from a file, running each line as a command"){
		File* file = file_init(args[0], FileAccess_Read);
		if(file){
			defer{ file_deinit(file); };
			
			char buffer[1024];
			while(str8 line = file_read_line(file, buffer, sizeof(buffer))){
				str8_advance_while(&line, ' '); //skip leading whitespace
				if(!line || str8_index(line, 0).codepoint == '#') continue; //skip empty lines and comments
				cmd_run(line);
			}
		}
	}DESHI_CMD_END(exec, CmdArgument_String);
	
	DESHI_CMD_START(file_exists, "Checks if a file exists"){
		Log("cmd","File '",args[0],"' ",(file_exists(args[0])) ? "exists." : "does not exist.");
	}DESHI_CMD_END(file_exists, CmdArgument_String);
	
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
	
	DESHI_CMD_START(mat_list, "Lists the materials and their info"){
		Log("", "Material List:");
		forI(array_count(assets_material_map())){
			Material* mat = assets_material_map()[i];
			Log("", " --- ", mat->name, " --- "
				"\n  Shaders:");
			Log("", "   Vertex: ", mat->stages.vertex->name);
			if(mat->stages.geometry)
				Log("", "   Geometry: ", mat->stages.geometry->name);
			if(mat->stages.fragment)
				Log("", "   Fragment: ", mat->stages.fragment->name);
			if(mat->texture_array) {
				Log("", "  Textures: ");
				forI(array_count(mat->texture_array)) {
					Log("", "   ", mat->texture_array[i]->name);
				}
			}
		}
	}DESHI_CMD_END_NO_ARGS(mat_list);
	
	DESHI_CMD_START(mat_shader, "Changes the shader of a material"){
		Material* mat = assets_material_get_by_name(args[0]);
		if(!mat){
			LogE("cmd","Could not find a material named '",args[0],"'.");
			return;
		}
		
		Shader* shader = assets_shader_get_by_name(args[1]);
		if(!shader) {
			LogE("cmd", "Could not find a shader named '", args[1], "'.");
			return;
		}
		
		str8 shader_type;
		
		switch(shader->type) {
			case ShaderType_Vertex: {
				shader_type = str8l("vertex");
				mat->stages.vertex = shader;
				mat->pipeline->vertex_shader = shader->handle;
			} break;
			case ShaderType_Geometry: {
				shader_type = str8l("geometry");
				mat->stages.geometry = shader;
				mat->pipeline->geometry_shader = shader->handle;
			} break;
			case ShaderType_Fragment: {
				shader_type = str8l("fragment");
				mat->stages.fragment = shader;
				mat->pipeline->fragment_shader = shader->handle;
			} break;
		}
		
		Log("cmd", "Updated material ", mat->name, "'s ", shader_type, " shader to ", shader->name, ".");
		graphics_pipeline_update(mat->pipeline);
	}DESHI_CMD_END(mat_shader, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(mat_texture, "Changes a texture of a material"){
		Material* mat = assets_material_get_by_name(args[0]);
		if(!mat){
			LogE("cmd","Could not find a material named '", args[0], "'.");
			return;
		}
		
		if(!mat->texture_array){
			LogE("cmd","The material '",args[0],"' has no texture slots.");
			return;
		}
		
		s32 texSlot = atoi(temp_str8_cstr(args[1]));
		if(texSlot < 0 || texSlot >= array_count(mat->texture_array)){
			LogE("cmd", "Given texture index '",texSlot,"' is outside of bounds '0..",array_count(mat->texture_array),"'.");
			return;
		}
		
		Texture* tex = assets_texture_get_by_name(args[2]);
		if(!tex){
			LogE("cmd", "Could not find a texture named '", args[2], "'.");
			return;
		}
		
		mat->texture_array[texSlot] = tex;
		Log("cmd", "Updated material ",mat->name,"'s texture",texSlot," to ",tex->name);
	}DESHI_CMD_END(mat_texture, CmdArgument_String, CmdArgument_S32, CmdArgument_String);
	
	DESHI_CMD_START(quit, "Exits the application"){
		platform_exit();
	}DESHI_CMD_END_NO_ARGS(quit);
	
	DESHI_CMD_START(rename, "Renames a file"){
		file_rename(args[0], args[1]);
	}DESHI_CMD_END(rename, CmdArgument_String, CmdArgument_String);
	
	DESHI_CMD_START(rm, "Remove a file"){
		file_delete(args[0], FileDeleteFlags_File);
	}DESHI_CMD_END(rm, CmdArgument_String);
	
	DESHI_CMD_START(shader_list, "Lists the shaders and their info"){
		Log("", "Shaders:");
		forI(array_count(assets_shader_map())) {
			Shader* s = assets_shader_map()[i];
			Log("", " --- ", s->name, " --- ",
				"\n  Type: ", ShaderTypeStrings[s->type]);
			if(s->resources && array_count(s->resources)) {
				Log("", "  Resources:");
				forI(array_count(s->resources)) {
					Log("", "   ", ShaderResourceTypeStrings[s->resources[i]]);
				}
			}
		}
	}DESHI_CMD_END_NO_ARGS(shader_list);
	
	DESHI_CMD_START(shader_reload, "Reloads specified shader"){
		Shader* shader = assets_shader_get_by_name(args[0]);
		if(!shader) {
			LogE("cmd", "Could not find shader with the name '", args[0], "'.");
			return;
		}
		
		assets_shader_reload(shader);
	}DESHI_CMD_END(shader_reload, CmdArgument_String);
	
	DESHI_CMD_START(show_ui_demo, "Toggles the visibility of the UI demo window"){
		ui_toggle_demo_window();
	}DESHI_CMD_END_NO_ARGS(show_ui_demo);
	
	DESHI_CMD_START(show_ui_debug, "Toggles the visibility of the UI debug window"){
		ui_toggle_debug_window();
	}DESHI_CMD_END_NO_ARGS(show_ui_debug);
	
	DESHI_CMD_START(texture_list, "Lists the textures and their info"){
		Log("", "Textures: ");
		forI(array_count(assets_texture_map())) {
			Texture* t = assets_texture_map()[i];
			Log("", " --- ", t->name, " --- "
				"\n  Dimensions: (", t->width, ", ", t->height, ")",
				"\n  Channels: ", t->depth, 
				"\n  Mipmaps: ", t->mipmaps, 
				"\n  Format: ", ImageFormatStrings[t->format],
				"\n  Type: ", TextureTypeStrings[t->type],
				"\n  Filter: ", TextureFilterStrings[t->filter],
				"\n  AddressMode: ", TextureAddressModeStrings[t->uv_mode]);
		}
	}DESHI_CMD_END_NO_ARGS(texture_list);
	
	DESHI_CMD_START(texture_load, "Loads a specific texture"){
		Stopwatch load_stopwatch = start_stopwatch();
		assets_texture_create_from_path_simple(args[0], args[1]);
		Log("cmd", "Loaded texture '",args[0],"' in ",peek_stopwatch(load_stopwatch),"ms");
	}DESHI_CMD_END(texture_load, CmdArgument_String, CmdArgument_String);
	
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
	
	DESHI_CMD_START(window_set_title, "Changes the title of the active window"){
		window_set_title(window_active, args[0]);
		Log("cmd","Updated active window's title to: ",args[0]);
	}DESHI_CMD_END(window_set_title, CmdArgument_String);
	
	DeshiStageInitEnd(DS_CMD);
}

#undef DESHI_CMD_START
#undef DESHI_CMD_END_NO_ARGS
#undef DESHI_CMD_END
