/* deshi Config Module
Index:
  ConfigValueType
  ConfigMapItem
  config_save(str8 path, ConfigMapItem* config_map, u64 config_count) -> void
  config_load(str8 path, ConfigMapItem* config_map, u64 config_count) -> void
*/

#pragma once
#ifndef DESHI_CONFIG_H
#define DESHI_CONFIG_H

#include "kigu/common.h"
#include "kigu/common.h"

typedef Type ConfigValueType; enum{
	ConfigValueType_NONE, //can be used for comments
	ConfigValueType_Bool,
	ConfigValueType_S32,
	ConfigValueType_U8,
	ConfigValueType_U32,
	ConfigValueType_F32, 
	ConfigValueType_F64, 
	ConfigValueType_FV2, 
	ConfigValueType_FV3,
	ConfigValueType_FV4,
	ConfigValueType_Str8, 
	ConfigValueType_KeyMod,
	
	//control enums
	ConfigValueType_PADSECTION,
	ConfigValueType_COUNT,
	
	//enum renames
	ConfigValueType_Int    = ConfigValueType_S32,
	ConfigValueType_Float  = ConfigValueType_F32,
	ConfigValueType_Double = ConfigValueType_F64,
	ConfigValueType_Vec2 = ConfigValueType_FV2,
	ConfigValueType_Vec3 = ConfigValueType_FV3,
	ConfigValueType_Vec4 = ConfigValueType_FV4,
};

struct ConfigMapItem{
	str8 key;
	ConfigValueType type;
	void* var;
};

//Saves the `config_map` to the `path`
global_ void config_save(str8 path, ConfigMapItem* config_map, u64 config_count);

//Loads the `config_map` from the `path`
global_ void config_load(str8 path, ConfigMapItem* config_map, u64 config_count);

#endif //DESHI_CONFIG_H
#ifdef DESHI_IMPLEMENTATION

void
config_save(str8 path, ConfigMapItem* config_map, u64 config_count){
	File* file = file_init(path, FileAccess_WriteTruncateCreate);
	if(!file) return;
	defer{ file_deinit(file); };
	
	persist u8 padding_buffer[33] = u8"                                ";
	string conversion;
	
	s64 pad_amount = 1;
	forI(config_count){
		//write key
		file_write(file, config_map[i].key.str, config_map[i].key.count);
		
		//write padding
		file_write(file, padding_buffer, (pad_amount > config_map[i].key.count) ? pad_amount-config_map[i].key.count : 1);
		
		//write value
		switch(config_map[i].type){
			case ConfigValueType_NONE:{
				//do nothing
			}break;
			case ConfigValueType_PADSECTION:{
				pad_amount = (config_map[i].var) ? ClampMax((s64)config_map[i].var, ArrayCount(padding_buffer)-1) : 1;
			}break;
			case ConfigValueType_Bool:{
				if(*(b32*)config_map[i].var){
					file_write(file, "true",  4);
				}else{
					file_write(file, "false", 5);
				}
			}break;
			case ConfigValueType_S32:{
				conversion = to_string(*(s32*)config_map[i].var, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_U8:{
				conversion = to_string((u32)(*(s8*)config_map[i].var), deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_U32:{
				conversion = to_string(*(u32*)config_map[i].var, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_F32:{
				conversion = to_string(*(f32*)config_map[i].var, true, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_F64:{
				conversion = to_string(*(f64*)config_map[i].var, true, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_FV2:{
				conversion = to_string(*(vec2*)config_map[i].var, true, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_FV3:{
				conversion = to_string(*(vec3*)config_map[i].var, true, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_FV4:{
				conversion = to_string(*(vec4*)config_map[i].var, true, deshi_temp_allocator);
				file_write(file, conversion.str, conversion.count);
			}break;
			case ConfigValueType_Str8:{
				file_write(file, "\"", 1);
				file_write(file, (*(str8*)config_map[i].var).str, (*(str8*)config_map[i].var).count);
				file_write(file, "\"", 1);
			}break;
			case ConfigValueType_KeyMod:{
				u32 key = (*(KeyCode*)config_map[i].var) & INPUT_KEY_MASK;
				u32 mod = (*(KeyCode*)config_map[i].var) & INPUT_MOD_MASK;
				file_write(file, KeyCodeStrings[key].str, KeyCodeStrings[key].count);
				switch(mod){
					case(InputMod_Any):            break; //append nothing
					case(InputMod_None):           file_write(file, "+none", 5); break;
					case(InputMod_Lctrl):          file_write(file, "+lctrl", 6); break;
					case(InputMod_Rctrl):          file_write(file, "+rctrl", 6); break;
					case(InputMod_Lshift):         file_write(file, "+lshift", 7); break;
					case(InputMod_Rshift):         file_write(file, "+rshift", 7); break;
					case(InputMod_Lalt):           file_write(file, "+lalt", 5); break;
					case(InputMod_Ralt):           file_write(file, "+ralt", 5); break;
					case(InputMod_LctrlLshift):    file_write(file, "+lctrl+lshift", 13); break;
					case(InputMod_LctrlRshift):    file_write(file, "+lctrl+rshift", 13); break;
					case(InputMod_RctrlLshift):    file_write(file, "+rctrl+lshift", 13); break;
					case(InputMod_RctrlRshift):    file_write(file, "+rctrl+rshift", 13); break;
					case(InputMod_LctrlLalt):      file_write(file, "+lctrl+lalt", 11); break;
					case(InputMod_LctrlRalt):      file_write(file, "+lctrl+ralt", 11); break;
					case(InputMod_RctrlLalt):      file_write(file, "+rctrl+lalt", 11); break;
					case(InputMod_RctrlRalt):      file_write(file, "+rctrl+ralt", 11); break;
					case(InputMod_LshiftLalt):     file_write(file, "+lshift+lalt", 12); break;
					case(InputMod_LshiftRalt):     file_write(file, "+lshift+ralt", 12); break;
					case(InputMod_RshiftLalt):     file_write(file, "+rshift+lalt", 12); break;
					case(InputMod_RshiftRalt):     file_write(file, "+rshift+ralt", 12); break;
					case(InputMod_LctrlLshiftLalt):file_write(file, "+lctrl+lshift+lalt", 18); break;
					case(InputMod_LctrlLshiftRalt):file_write(file, "+lctrl+lshift+ralt", 18); break;
					case(InputMod_LctrlRshiftLalt):file_write(file, "+lctrl+rshift+lalt", 18); break;
					case(InputMod_LctrlRshiftRalt):file_write(file, "+lctrl+rshift+ralt", 18); break;
					case(InputMod_RctrlLshiftLalt):file_write(file, "+rctrl+lshift+lalt", 18); break;
					case(InputMod_RctrlLshiftRalt):file_write(file, "+rctrl+lshift+ralt", 18); break;
					case(InputMod_RctrlRshiftLalt):file_write(file, "+rctrl+rshift+lalt", 18); break;
					case(InputMod_RctrlRshiftRalt):file_write(file, "+rctrl+rshift+ralt", 18); break;
					default: LogE("config","Unknown key-modifier combination '",(*(KeyCode*)config_map[i].var),"' when saving config: ",path);
				} 
			}break;
			default:{
				LogE("config","Unhandled value type when saving config '",path,"' for key: ",config_map[i].key);
				NotImplemented;
			}break;
		}
		
		file_write(file, "\n", 1);
	}
}

void
config_load(str8 path, ConfigMapItem* config_map, u64 config_count){
	File* file = file_init_if_exists(path, FileAccess_Read);
	if(!file){ config_save(path, config_map, config_count); return; };
	defer{ file_deinit(file); };
	
	//NOTE(delle) creating an allocator here to either use 256 bytes locally or temp allocate more than 256 bytes
	persist u8 line_buffer[256];
	persist Allocator load_allocator{
		[](upt bytes){
			if(bytes > 256){
				return memory_talloc(bytes);
			}else{
				line_buffer[bytes-1] = '\0'; //NOTE(delle) file_read_line_alloc() requests an extra byte for null-terminator
				return (void*)line_buffer;
			}
		},
		Allocator_ChangeMemory_Noop,
		Allocator_ChangeMemory_Noop,
		Allocator_ReleaseMemory_Noop,
		Allocator_ResizeMemory_Noop
	};
	
	//copy the config map so we don't have to check keys twice
	//TODO(delle) performance check that this copy is cheaper than repeated key string comparing
	ConfigMapItem* configs = (ConfigMapItem*)memory_talloc(config_count*sizeof(ConfigMapItem));
	CopyMemory(configs, config_map, config_count*sizeof(ConfigMapItem));
	
	str8 line;
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		line = file_read_line_alloc(file, &load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		if(decoded_codepoint_from_utf8(line.str, 4).codepoint == '#'){
			continue;
		}
		
		//parse key
		str8 key = str8_eat_until(line, ' ');
		str8_increment(&line, key.count);
		
		//skip separating whitespace
		str8_advance_while(&line, ' ');
		if(!line){
			LogE("config","Error parsing '",path,"' on line ",line_number,". No value passed to key: ",key);
			continue;
		}
		
		//early out if comment is first value character
		if(decoded_codepoint_from_utf8(line.str, 4).codepoint == '#'){
			LogE("config","Error parsing '",path,"' on line ",line_number,". No value passed to key: ",key);
			continue;
		}
		
		//parse value based on key type
		forI(config_count){
			if(configs[i].type == ConfigValueType_NONE || configs[i].type == ConfigValueType_PADSECTION) continue;
			if(!str8_equal_lazy(key, configs[i].key)) continue;
			
			switch(configs[i].type){
				case ConfigValueType_Bool:{
					b32* b = (b32*)configs[i].var;
					if     (str8_nequal(str8_lit("true"),  line, 4)) *b = true;
					else if(str8_nequal(str8_lit("false"), line, 5)) *b = false;
					else if(str8_nequal(str8_lit("1"),     line, 1)) *b = true;
					else if(str8_nequal(str8_lit("0"),     line, 1)) *b = false;
					else LogE("config","Error parsing '",path,"' on line ",line_number,". Invalid boolean value: ",line);
				}break;
				case ConfigValueType_S32:{
					*(s32*)configs[i].var = atoi((const char*)line.str);
				}break;
				case ConfigValueType_U8:{
					*(u8*)configs[i].var = (u8)atoi((const char*)line.str);
				}break;
				case ConfigValueType_U32:{
					*(u32*)configs[i].var = (u32)atoi((const char*)line.str);
				}break;
				case ConfigValueType_F32:{
					*(f32*)configs[i].var = atof((const char*)line.str);
				}break;
				case ConfigValueType_F64:{
					*(f64*)configs[i].var = atof((const char*)line.str);
				}break;
				case ConfigValueType_FV2:{
					vec2* vec = (vec2*)configs[i].var;
					char* cursor = (char*)line.str;
					vec->x = strtof(cursor+1,&cursor); //NOTE(delle) start after the (
					vec->y = strtof(cursor+1,0);
				}break;
				case ConfigValueType_FV3:{
					vec3* vec = (vec3*)configs[i].var;
					char* cursor = (char*)line.str;
					vec->x = strtof(cursor+1,&cursor); //NOTE(delle) start after the (
					vec->y = strtof(cursor+1,&cursor);
					vec->z = strtof(cursor+1,0);
				}break;
				case ConfigValueType_FV4:{
					vec4* vec = (vec4*)configs[i].var;
					char* cursor = (char*)line.str;
					vec->x = strtof(cursor+1,&cursor); //NOTE(delle) start after the (
					vec->y = strtof(cursor+1,&cursor);
					vec->z = strtof(cursor+1,&cursor);
					vec->w = strtof(cursor+1,0);
				}break;
				case ConfigValueType_Str8:{ //TODO(delle) !Leak can't free if we don't know if str8 is based on a literal (use str8_builder)
					//if the value starts with a double-quote, parse until next double-quote, else parse until end of line
					if(decoded_codepoint_from_utf8(line.str,4).codepoint == '\"'){
						str8_increment(&line, 1);
						str8 str = str8_eat_until(line, '\"');
						*(str8*)configs[i].var = str8_copy(str,  deshi_allocator);
					}else{
						*(str8*)configs[i].var = str8_copy(line, deshi_allocator);
					}
				}break;
				case ConfigValueType_KeyMod:{
					KeyCode* keybind = (KeyCode*)configs[i].var;
					
					//parse keybind key
					str8 keybind_key = str8_eat_until(line, '+');
					if(*keybind_key.str >= 97  && *keybind_key.str <= 122) *keybind_key.str -= 32; //NOTE(delle) uppercase first letter for convenience
					u32 key_index = -1;
					forI(ArrayCount(KeyCodeStrings)){
						if(str8_equal_lazy(KeyCodeStrings[i], keybind_key)){
							*keybind = i;
							key_index = i;
							break;
						}
					}
					if(key_index == -1){
						LogE("config","Error parsing '",path,"' on line ",line_number,". Invalid keybind key: ",keybind_key);
						continue;
					}
					
					//parse keybind mods
					str8_increment(&line, keybind_key.count+1);
					while(line){
						str8 keybind_mod = str8_eat_until(line, '+');
						if(keybind_mod.count < 2){
							LogE("config","Error parsing '",path,"' on line ",line_number,". Invalid keybind mod: ",keybind_mod);
						}else if(*keybind_mod.str == 'l'){
							if     (*(keybind_mod.str+1) == 'c') *keybind |= InputMod_Lctrl;
							else if(*(keybind_mod.str+1) == 's') *keybind |= InputMod_Lshift;
							else if(*(keybind_mod.str+1) == 'a') *keybind |= InputMod_Lalt;
							else LogE("config","Error parsing '",path,"' on line ",line_number,". Invalid keybind mod: ",keybind_mod);
						}else if(*keybind_mod.str == 'r'){
							if     (*(keybind_mod.str+1) == 'c') *keybind |= InputMod_Rctrl;
							else if(*(keybind_mod.str+1) == 's') *keybind |= InputMod_Rshift;
							else if(*(keybind_mod.str+1) == 'a') *keybind |= InputMod_Ralt;
							else LogE("config","Error parsing '",path,"' on line ",line_number,". Invalid keybind mod: ",keybind_mod);
						}else if(*keybind_mod.str == 'n'){
							//overwrite any other input mods since they aren't compatible
							*keybind = (key_index | InputMod_None);
							break;
						}else{
							LogE("config","Error parsing '",path,"' on line ",line_number,". Invalid keybind mod: ",keybind_mod);
						}
						str8_increment(&line, keybind_mod.count+1);
					}
				}break;
				default:{
					LogE("config","Unhandled value type when loading config '",path,"' for key: ",configs[i].key);
					NotImplemented;
				}break;
			}
			
			//set type to NONE so this key won't get checked for again
			configs[i].type = ConfigValueType_NONE;
		}
	}
}

#endif //DESHI_IMPLEMENTATION
#ifdef DESHI_TESTS



#endif //DESHI_TESTS