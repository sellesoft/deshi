//NOTE(delle) these are used in logger_init() for setting Win32 stdout mode to UTF16
#include <io.h>
#include <fcntl.h>
#include <ctime>
#include <clocale>

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @internal
local Logger logger;

int
logger_message_prefix(int cursor, str8 caller_file, upt caller_line, str8 tag, Type log_type){DPZoneScoped;
	//tag
	if(tag && !logger.ignore_tags){
		logger.last_message[cursor++] = '[';
		if     (log_type==LogType_Error)  {memcpy(logger.last_message+cursor, "\x1b[31m", 5); cursor+=5;}
		else if(log_type==LogType_Success){memcpy(logger.last_message+cursor, "\x1b[32m", 5); cursor+=5;}
		else if(log_type==LogType_Warning){memcpy(logger.last_message+cursor, "\x1b[33m", 5); cursor+=5;}

		str8 temp = tag;
		while(temp){
			cursor += utf8_from_codepoint(logger.last_message + cursor, towupper(str8_advance(&temp).codepoint));
		}
		switch(log_type){
			case LogType_Normal: { memcpy(logger.last_message + cursor, "] "        ,         2*sizeof(u8)); cursor +=  2; }break;
			case LogType_Error:  { memcpy(logger.last_message + cursor, "-ERROR\x1b[0m] "  , 12*sizeof(u8)); cursor += 12; }break;
			case LogType_Warning:{ memcpy(logger.last_message + cursor, "-WARNING\x1b[0m] ", 14*sizeof(u8)); cursor += 14; }break;
			case LogType_Success:{ memcpy(logger.last_message + cursor, "-SUCCESS\x1b[0m] ", 14*sizeof(u8)); cursor += 14; }break;
		}
	}else if(log_type != LogType_Normal){
		logger.last_message[cursor++] = '[';
		switch(log_type){
			case LogType_Error:  { memcpy(logger.last_message + cursor, "\x1b[31mERROR\x1b[0m] "  , 16*sizeof(u8)); cursor += 16; }break;
			case LogType_Success:{ memcpy(logger.last_message + cursor, "\x1b[32mSUCCESS\x1b[0m] ", 18*sizeof(u8)); cursor += 18; }break;
			case LogType_Warning:{ memcpy(logger.last_message + cursor, "\x1b[33mWARNING\x1b[0m] ", 18*sizeof(u8)); cursor += 18; }break;
		}
	}
	
	//indentation
	if(logger.indent_level && logger.indent_spaces){
		cursor += stbsp_snprintf((char*)logger.last_message + cursor, LOGGER_BUFFER_SIZE - cursor,
								 "%*s", logger.indent_level * logger.indent_spaces, "");
	}
	
	//caller file and line
	if(logger.track_caller){
		cursor += stbsp_snprintf((char*)logger.last_message + cursor, LOGGER_BUFFER_SIZE - cursor,
								 "%s(%lld): ", (char*)caller_file.str, caller_line);
	}
	
	return cursor;
}

int
logger_message_postfix(int cursor, str8 tag, Type log_type){DPZoneScoped;
	//write to stdout (convert to wide characters b/c MSVC/Windows requires it)
	if(logger.mirror_to_stdout){
		persist wchar_t logger_conversion_buffer[LOGGER_BUFFER_SIZE] = {0};
		logger.last_message[cursor] = '\0';
		mbstowcs(logger_conversion_buffer, (char*)logger.last_message, LOGGER_BUFFER_SIZE-1);
		fputws(logger_conversion_buffer, stdout);
		if(logger.auto_newline) fputwc('\n', stdout);
	}
	
	//automatically append newline
	if(logger.auto_newline){
		if(cursor+1 >= LOGGER_BUFFER_SIZE) cursor -= 1;
		logger.last_message[cursor+0] = '\n';
		logger.last_message[cursor+1] = '\0';
		cursor += 1;
	}
	logger.last_message_length = cursor;
	
	
	//write to log file
	u32 log_file_offset = logger.file->bytes;
	if(logger.mirror_to_file){
		file_append(logger.file, logger.last_message, logger.last_message_length);
	}
	
	//write to console
	if(logger.mirror_to_console && DeshiModuleLoaded(DS_CONSOLE)){
		ConsoleChunk chunk;
		chunk.tag = tag;
		
		u32 message_offset = tag.count;
		switch(log_type){
			case LogType_Normal: { chunk.type = ConsoleChunkType_Normal;  chunk.color = Color_White;  message_offset += 0; }break; //"[] "
			case LogType_Error:  { chunk.type = ConsoleChunkType_Error;   chunk.color = Color_Red;    message_offset += 0; }break; //"[] " and "-ERROR"
			case LogType_Warning:{ chunk.type = ConsoleChunkType_Warning; chunk.color = Color_Yellow; message_offset += 0; }break; //"[] " and "-WARNING"
			case LogType_Success:{ chunk.type = ConsoleChunkType_Success; chunk.color = Color_Green;  message_offset += 0; }break; //"[] " and "-SUCCESS"
		}
		
		chunk.start   = log_file_offset + message_offset;
		chunk.size    = logger.last_message_length - message_offset;
		chunk.newline = (logger.last_message[logger.last_message_length-1] == '\n');
		if(chunk.newline) chunk.size -= 1;
		console_expose()->dictionary.add(chunk);
	}
	
	return cursor;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @interface
void
logger_init(u32 log_count, b32 mirror){DPZoneScoped;
	DeshiStageInitStart(DS_LOGGER, DS_PLATFORM, "Attempted to initialize Logger module before initializing the Platform module");
	
	//create the logs directory if it doesn't exist already
	file_create(str8_lit("data/logs/"));
	
	logger.mirror_to_file    = true;
	logger.mirror_to_stdout  = mirror;
	logger.mirror_to_console = false; //NOTE(delle) this gets set to true when Console is init
	
	u8 path_buffer[256];
	log_count = ClampMin(log_count, 1);
	carray<File> log_files = file_search_directory(str8_lit("data/logs/"));
	
	//rename previous log.txt
	forE(log_files){
		if(str8_equal(it->name, str8_lit("log.txt"))){
			int len = stbsp_snprintf((char*)path_buffer, ArrayCount(path_buffer), "data/logs/log_%lld.txt", it->last_write_time);
			file_rename(it->path, (str8{path_buffer, (s64)len}));
			it->path = str8{path_buffer, (s64)len}; //NOTE(delle) correcting path on temp memory so it's valid for deletion
		}
	}
	
	//delete all but last 'log_count' files in logs directory
	if(log_files.count > log_count){
		//sort logs ascending based on last write time
		b32 swapped = false;
		forX(i,log_count){
			swapped = false;
			forX(j,log_files.count-1-i){
				if(log_files[j].last_write_time > log_files[j+1].last_write_time){
					Swap(log_files[j], log_files[j+1]);
					swapped = true;
				}
			}
			if(!swapped) break;
		}
		
		//delete logs
		forI((log_files.count-log_count)+1) file_delete(log_files[i].path);
	}
	
	//create log file named as current time
	logger.file = file_init(str8_lit("data/logs/log.txt"), FileAccess_ReadWriteAppendCreate);
	Assert(logger.file, "logger failed to open file");
	
	//write date at top of file
	time_t rawtime = time(0);
	int len = strftime((char*)path_buffer, ArrayCount(path_buffer), "%c", localtime(&rawtime));
	file_append(logger.file, path_buffer, len);
	file_append(logger.file, (void*)"\n\n", 2);
	
#if BUILD_SLOW //NOTE(delle) write immediately when debugging so that a Log() right before Assert() still writes
	setvbuf(logger.file->handle,0,_IONBF,0);
#endif
	
	fflush(stdout); _setmode(_fileno(stdout), _O_U16TEXT); //NOTE(delle) enables Unicode printing to stdout on Windows
	setlocale(LC_ALL, ".utf8");
	
	DeshiStageInitEnd(DS_LOGGER);
}

void
logger_update(){DPZoneScoped;
	//TODO maybe flush every X seconds/frames instead of every update?
	int error = fflush(logger.file->handle);
	Assert(error == 0, "logger failed to flush file");
}

void
logger_cleanup(){DPZoneScoped;
	file_deinit(logger.file);
	fflush(stdout); _setmode(_fileno(stdout), _O_TEXT); //NOTE(delle) disable Unicode printing to stdout on Windows
	logger.file = 0;
}

Logger*
logger_expose(){DPZoneScoped;
	return &logger;
}

void
logger_push_indent(s32 count){DPZoneScoped;
	logger.indent_level += count;
}

void
logger_pop_indent(s32 count){DPZoneScoped;
	logger.indent_level = (count < 0) ? 0 : ClampMin(logger.indent_level - count, 0);
}

str8
logger_last_message(){DPZoneScoped;
	return str8{logger.last_message,logger.last_message_length};
}

void
logger_format_log(str8 caller_file, upt caller_line, str8 tag, Type log_type, str8 fmt, ...){DPZoneScoped;
	if(!logger.file) return;
	
	int cursor = logger_message_prefix(0, caller_file, caller_line, tag, log_type);
	va_list args;
	va_start(args, fmt);
	cursor += stbsp_vsnprintf((char*)logger.last_message + cursor, LOGGER_BUFFER_SIZE - cursor, (char*)fmt.str, args);
	va_end(args);
	logger_message_postfix(cursor, tag, log_type);
}

void
logger_comma_log_internal(str8 caller_file, upt caller_line, str8 tag, Type log_type, string* arr, u32 arr_count){DPZoneScoped;
	if(!logger.file) return;
	
	int cursor = logger_message_prefix(0, caller_file, caller_line, tag, log_type);
	forI(arr_count){
		if(cursor+arr[i].count >= LOGGER_BUFFER_SIZE){
			LogW("logger","Attempted to log a message more than ", LOGGER_BUFFER_SIZE, " characters long.");
			break;
		}
		memcpy(logger.last_message + cursor, arr[i].str, arr[i].count*sizeof(char));
		cursor += arr[i].count;
	}
	logger_message_postfix(cursor, tag, log_type);
}