//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @internal
local Logger deshi__logger;

int logger_message_prefix(int cursor, str8 caller_file, upt caller_line, str8 tag, Type log_type){DPZoneScoped;
	//tag
	if(tag && !deshi__logger.ignore_tags){
		deshi__logger.last_message[cursor++] = '[';
		str8 temp = tag;
		while(temp){
			cursor += utf8_from_codepoint(deshi__logger.last_message + cursor, towupper(str8_advance(&temp).codepoint));
		}
		switch(log_type){
			case LogType_Normal: { memcpy(deshi__logger.last_message + cursor, "] "        ,  2*sizeof(u8)); cursor +=  2; }break;
			case LogType_Error:  { memcpy(deshi__logger.last_message + cursor, "-ERROR] "  ,  8*sizeof(u8)); cursor +=  8; }break;
			case LogType_Warning:{ memcpy(deshi__logger.last_message + cursor, "-WARNING] ", 10*sizeof(u8)); cursor += 10; }break;
			case LogType_Success:{ memcpy(deshi__logger.last_message + cursor, "-SUCCESS] ", 10*sizeof(u8)); cursor += 10; }break;
		}
	}else if(log_type != LogType_Normal){
		deshi__logger.last_message[cursor++] = '[';
		switch(log_type){
			case LogType_Error:  { memcpy(deshi__logger.last_message + cursor, "ERROR] "  , 7*sizeof(u8)); cursor += 7; }break;
			case LogType_Warning:{ memcpy(deshi__logger.last_message + cursor, "WARNING] ", 9*sizeof(u8)); cursor += 9; }break;
			case LogType_Success:{ memcpy(deshi__logger.last_message + cursor, "SUCCESS] ", 9*sizeof(u8)); cursor += 9; }break;
		}
	}
	
	//indentation
	if(deshi__logger.indent_level && deshi__logger.indent_spaces){
		cursor += stbsp_snprintf((char*)deshi__logger.last_message + cursor, LOGGER_BUFFER_SIZE - cursor,
								 "%*s", deshi__logger.indent_level * deshi__logger.indent_spaces, "");
	}
	
	//caller file and line
	if(deshi__logger.track_caller){
		cursor += stbsp_snprintf((char*)deshi__logger.last_message + cursor, LOGGER_BUFFER_SIZE - cursor,
								 "%s(%lld): ", (char*)caller_file.str, caller_line);
	}
	
	return cursor;
}

int logger_message_postfix(int cursor, str8 tag, Type log_type){DPZoneScoped;
	//write to stdout (convert to wide characters b/c MSVC/Windows requires it)
	if(deshi__logger.mirror_to_stdout){
		persist wchar_t deshi__logger_conversion_buffer[LOGGER_BUFFER_SIZE] = {0};
		deshi__logger.last_message[cursor] = '\0';
		mbstowcs(deshi__logger_conversion_buffer, (char*)deshi__logger.last_message, LOGGER_BUFFER_SIZE-1);
		fputws(deshi__logger_conversion_buffer, stdout);
		if(deshi__logger.auto_newline) fputwc('\n', stdout);
	}
	
	//automatically append newline
	if(deshi__logger.auto_newline){
		if(cursor+1 >= LOGGER_BUFFER_SIZE) cursor -= 1;
		deshi__logger.last_message[cursor+0] = '\n';
		deshi__logger.last_message[cursor+1] = '\0';
		cursor += 1;
	}
	deshi__logger.last_message_length = cursor;
	
	//write to log file
	u32 log_file_offset = ftell(deshi__logger.file);
	fputs((char*)deshi__logger.last_message, deshi__logger.file);
	
	//write to console
	if(deshi__logger.mirror_to_console && DeshiModuleLoaded(DS_CONSOLE)){
		ConsoleChunk chunk;
		chunk.tag = tag;
		
		u32 message_offset = tag.count;
		switch(log_type){
			case LogType_Normal: { chunk.type = ConsoleChunkType_Normal;  chunk.color = Color_White;  message_offset +=  3; }break; //"[] "
			case LogType_Error:  { chunk.type = ConsoleChunkType_Error;   chunk.color = Color_Red;    message_offset +=  9; }break; //"[] " and "-ERROR"
			case LogType_Warning:{ chunk.type = ConsoleChunkType_Warning; chunk.color = Color_Yellow; message_offset += 11; }break; //"[] " and "-WARNING"
			case LogType_Success:{ chunk.type = ConsoleChunkType_Success; chunk.color = Color_Green;  message_offset += 11; }break; //"[] " and "-SUCCESS"
		}
		
		chunk.start   = log_file_offset + message_offset;
		chunk.size    = deshi__logger.last_message_length - message_offset;
		chunk.newline = (deshi__logger.last_message[deshi__logger.last_message_length-1] == '\n');
		if(chunk.newline) chunk.size -= 1;
		console_expose()->dictionary.add(chunk);
	}
	
	return cursor;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @interface
void logger_init(u32 log_count, b32 mirror){DPZoneScoped;
	DeshiStageInitStart(DS_LOGGER, DS_MEMORY, "Attempted to initialize Logger module before initializing Memory module");
	
	deshi__logger.mirror_to_stdout  = mirror;
	deshi__logger.mirror_to_console = false; //NOTE(delle) this gets set to true when Console is init
	
	char path_buffer[MAX_FILEPATH_SIZE];
	log_count = ClampMin(log_count, 1);
	array<File> log_files = get_directory_files("data/logs");
	
	//rename previous log.txt
	forE(log_files){
		if(strcmp(it->name, "log.txt") == 0){
			int len = stbsp_snprintf(path_buffer, MAX_FILEPATH_SIZE, "data/logs/log_%lld.txt", it->time_last_write);
			rename_file(it->path, path_buffer);
			memcpy(it->path, path_buffer, len*sizeof(char));
		}
	}
	
	//delete all but last 'log_count' files in logs directory
	if(log_files.count > log_count){
		//sort logs ascending based on last write time
		b32 swapped = false;
		forX(i,log_count){
			swapped = false;
			forX(j,log_files.count-1-i){
				if(log_files[j].time_last_write > log_files[j+1].time_last_write){
					Swap(log_files[j], log_files[j+1]);
					swapped = true;
				}
			}
			if(!swapped) break;
		}
		
		//delete logs
		forI((log_files.count-log_count)+1) delete_file(&log_files[i]);
	}
	
	//create log file named as current time
	time_t rawtime = time(0);
	stbsp_snprintf(path_buffer, MAX_FILEPATH_SIZE, "data/logs/log.txt", (u64)rawtime);
	deshi__logger.file = fopen(path_buffer,"ab+"); Assert(deshi__logger.file, "logger failed to open file");
	
	//write date at top of file
	strftime(path_buffer, MAX_FILEPATH_SIZE, "%c", localtime(&rawtime));
	fprintf(deshi__logger.file, "%s\n\n", path_buffer);
	
#if BUILD_SLOW //NOTE(delle) write immediately when debugging so that a Log() right before Assert() still writes
	setvbuf(deshi__logger.file,0,_IONBF,0);
#endif
	
	fflush(stdout); _setmode(_fileno(stdout), _O_U16TEXT); //NOTE(delle) enable Unicode printing to stdout on Windows
	setlocale(LC_ALL, ".utf8");
	
	DeshiStageInitEnd(DS_LOGGER);
}

void logger_update(){DPZoneScoped;
	//TODO maybe flush every X seconds/frames instead of every update?
	int error = fflush(deshi__logger.file);
	Assert(error == 0, "logger failed to flush file");
}

void logger_cleanup(){DPZoneScoped;
	fclose(deshi__logger.file);
	fflush(stdout); _setmode(_fileno(stdout), _O_TEXT); //NOTE(delle) disable Unicode printing to stdout on Windows
	deshi__logger.file = 0;
}

Logger* logger_expose(){DPZoneScoped;
	return &deshi__logger;
}

void logger_push_indent(s32 count){DPZoneScoped;
	deshi__logger.indent_level += count;
}

void logger_pop_indent(s32 count){DPZoneScoped;
	deshi__logger.indent_level = (count < 0) ? 0 : ClampMin(deshi__logger.indent_level - count, 0);
}

str8 logger_last_message(){DPZoneScoped;
	return str8{deshi__logger.last_message,deshi__logger.last_message_length};
}

void logger_format_log(str8 caller_file, upt caller_line, str8 tag, Type log_type, str8 fmt, ...){DPZoneScoped;
	if(!deshi__logger.file) return;
	
	int cursor = logger_message_prefix(0, caller_file, caller_line, tag, log_type);
	va_list args;
	va_start(args, fmt);
	cursor += stbsp_vsnprintf((char*)deshi__logger.last_message + cursor, LOGGER_BUFFER_SIZE - cursor, (char*)fmt.str, args);
	va_end(args);
	logger_message_postfix(cursor, tag, log_type);
}

void logger_comma_log_internal(str8 caller_file, upt caller_line, str8 tag, Type log_type, string* arr, u32 arr_count){DPZoneScoped;
	if(!deshi__logger.file) return;
	
	int cursor = logger_message_prefix(0, caller_file, caller_line, tag, log_type);
	forI(arr_count){
		if(cursor+arr[i].count >= LOGGER_BUFFER_SIZE){
			LogW("logger","Attempted to log a message more than 4096 characters long.");
			break;
		}
		memcpy(deshi__logger.last_message + cursor, arr[i].str, arr[i].count*sizeof(char));
		cursor += arr[i].count;
	}
	logger_message_postfix(cursor, tag, log_type);
}