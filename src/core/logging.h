#pragma once
#ifndef DESHI_LOGGING_H
#define DESHI_LOGGING_H

#include "console2.h"
#include "../defines.h"
#include "../utils/string.h"
#include "../utils/cstring.h"
#include "../utils/string_conversion.h"

//TODO look into https://github.com/fmtlib/fmt
//TODO can probably optimize by using a single buffer instead of strings in Log() and LogA()
//TODO add severity/levels to logging
////////////////////
//// @interface ////
////////////////////
#define LOG_BUFFER_SIZE 1024
#define LOG_PATH_SIZE 1024
#define Log(tag,...)       Logging::Log_(__FILE__,__LINE__,tag,__VA_ARGS__)           //comma style:  log("first ",12.5," f ",0xff)
#define LogE(tag,...)      Logging::Log_(__FILE__,__LINE__,GLUE(tag,"-error"),__VA_ARGS__)
#define LogW(tag,...)      Logging::Log_(__FILE__,__LINE__,GLUE(tag,"-warning"),__VA_ARGS__)
#define Logf(tag,fmt,...)  Logging::LogF_(__FILE__,__LINE__,tag,fmt,__VA_ARGS__) //printf style: logf("first %f f %d",12.5f,0xff)
#define LogfE(tag,fmt,...) Logging::LogF_(__FILE__,__LINE__,GLUE(tag,"-error"),fmt,__VA_ARGS__)
#define LogfW(tag,fmt,...) Logging::LogF_(__FILE__,__LINE__,GLUE(tag,"-warning"),fmt,__VA_ARGS__)
#define Loga(tag,fmt,...)  Logging::LogA_(__FILE__,__LINE__,tag,fmt,__VA_ARGS__) //auto style:   loga("first $ f $",12.5f,0xff)
#define LogaE(tag,fmt,...) Logging::LogA_(__FILE__,__LINE__,GLUE(tag,"-error"),fmt,__VA_ARGS__)
#define LogaW(tag,fmt,...) Logging::LogA_(__FILE__,__LINE__,GLUE(tag,"-warning"),fmt,__VA_ARGS__)

namespace Logging{
	local FILE* file = 0;
	local char log_buffer[LOG_BUFFER_SIZE] = {};
	local char log_path[LOG_PATH_SIZE] = {};
	local cstring last_message = {log_buffer,0};
	local bool mirror_to_stdout = true;
	local bool mirror_to_console = false;
	local bool log_file_and_line = false;
	
	template<typename... T> void Log_(const char* filepath, upt line_number, const char* tag, T... args);
	void LogF_(const char* filepath, upt line_number, const char* tag, const char* fmt, ...);
	template<typename... T> void LogA_(const char* filepath, upt line_number, const char* tag, const char* fmt, T... args);
	
	void Init(u32 log_count = 5, bool mirror = true, bool fileline = false);
	void Update();
	void Cleanup();
};

/////////////////////////
//// @implementation ////
/////////////////////////
template<typename... T> inline void Logging::
Log_(const char* filepath, upt line_number, const char* tag, T... args){
	string str = (tag && *tag != 0) ? "["+string::toUpper(tag)+"] " : "";
	if(log_file_and_line) str += to_string("%s(%zd): ", filepath,line_number);
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(std::forward<T>(args))...};
	forI(arg_count) str += arr[i];
	if(mirror_to_stdout) puts(str.str);
	str += "\n";
	fputs(str.str, file);
	memcpy(log_buffer, str.str, str.count);
	last_message.count = str.count;
	if(mirror_to_console) Console2::AddLog(str);
}

inline void Logging::
LogF_(const char* filepath, upt line_number, const char* tag, const char* fmt, ...){
	int cursor = (tag && *tag != 0) ? snprintf(log_buffer, LOG_BUFFER_SIZE, "[%s] ", string::toUpper(tag).str) : 0;
	if(log_file_and_line) cursor += snprintf(log_buffer+cursor, LOG_BUFFER_SIZE-cursor, "%s(%zd): ", filepath,line_number);
	va_list args; va_start(args, fmt);
	cursor += vsnprintf(log_buffer+cursor, LOG_BUFFER_SIZE-cursor, fmt, args);
	va_end(args);
	if(mirror_to_stdout) puts(log_buffer);
	cursor += snprintf(log_buffer+cursor, LOG_BUFFER_SIZE-cursor, "%s", "\n");
	fputs(log_buffer, file);
	last_message.count = cursor;
	if(mirror_to_console) Console2::AddLog(to_string(last_message));
}

template<typename... T> inline void Logging::
LogA_(const char* filepath, upt line_number, const char* tag, const char* fmt, T... args){
	string str = (tag && *tag != 0) ? "["+string::toUpper(tag)+"] " : "";
	if(log_file_and_line) str += to_string("%s(%zd): ", filepath,line_number);
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(std::forward<T>(args))...};
	
	const char* sub_start = fmt;
	const char* cursor = fmt;
	u32 special_count = 0;
	while(*cursor != '\0'){
		if(*cursor == '$'){
			if(*(cursor+1) != '$'){
				str += string(sub_start,cursor-sub_start);
				Assert(special_count < arg_count, "more $ than args");
				str += to_string(arr[special_count]);
				sub_start = cursor+1;
			}else{
				str += "$";
				sub_start = cursor+2;
				cursor++;
			}
			special_count++;
		}
		cursor++;
	}
	if(special_count != arg_count) Log("logging","More arguments passed to loga() than $ characters in the format string");
	
	str += string(sub_start,cursor-sub_start);
	if(mirror_to_stdout) puts(str.str);
	str += "\n";
	memcpy(log_buffer, str.str, str.count);
	last_message.count = str.count;
	if(mirror_to_console) Console2::AddLog(str);
}

#endif //DESHI_LOGGING_H
