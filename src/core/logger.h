#pragma once
#ifndef DESHI_LOGGING_H
#define DESHI_LOGGING_H

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
#define Log(tag,...)       Logger::Log_(__FILE__,__LINE__,tag,__VA_ARGS__)           //comma style:  log("first ",12.5," f ",0xff)
#define LogE(tag,...)      Logger::Log_(__FILE__,__LINE__,GLUE(tag,"-error"),__VA_ARGS__)
#define LogW(tag,...)      Logger::Log_(__FILE__,__LINE__,GLUE(tag,"-warning"),__VA_ARGS__)
#define Logf(tag,fmt,...)  Logger::LogF_(__FILE__,__LINE__,tag,fmt,__VA_ARGS__) //printf style: logf("first %f f %d",12.5f,0xff)
#define LogfE(tag,fmt,...) Logger::LogF_(__FILE__,__LINE__,GLUE(tag,"-error"),fmt,__VA_ARGS__)
#define LogfW(tag,fmt,...) Logger::LogF_(__FILE__,__LINE__,GLUE(tag,"-warning"),fmt,__VA_ARGS__)
#define Loga(tag,fmt,...)  Logger::LogA_(__FILE__,__LINE__,tag,fmt,__VA_ARGS__) //auto style:   loga("first $ f $",12.5f,0xff)
#define LogaE(tag,fmt,...) Logger::LogA_(__FILE__,__LINE__,GLUE(tag,"-error"),fmt,__VA_ARGS__)
#define LogaW(tag,fmt,...) Logger::LogA_(__FILE__,__LINE__,GLUE(tag,"-warning"),fmt,__VA_ARGS__)

#define EnableLogging  Logger::SetIsLogging(1);
#define DisableLogging Logger::SetIsLogging(0);

namespace Logger{
	template<typename... T> void Log_(const char* filepath, upt line_number, const char* tag, T... args);
	void LogF_(const char* filepath, upt line_number, const char* tag, const char* fmt, ...);
	template<typename... T> void LogA_(const char* filepath, upt line_number, const char* tag, const char* fmt, T... args);
	
	void LogInternal(string& msg);
	cstring LastMessage();
	
	void LogFromConsole(string& in);
	
	void SetIsLogging(b32 thebooleanforthisfunction);
	
	FILE* GetFilePtr();
	
	void Init(u32 log_count = 5, b32 mirror = true);
	void Update();
	void Cleanup();
};

/////////////////////////
//// @implementation ////
/////////////////////////
template<typename... T> inline void Logger::
Log_(const char* filepath, upt line_number, const char* tag, T... args){
	string str = (tag && *tag != 0) ? "["+string::toUpper(tag)+"] " : "";
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(std::forward<T>(args))...};
	forI(arg_count) str += arr[i];
	LogInternal(str);
}

template<typename... T> inline void Logger::
LogA_(const char* filepath, upt line_number, const char* tag, const char* fmt, T... args){
	string str = (tag && *tag != 0) ? "["+string::toUpper(tag)+"] " : "";
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
	str += string(sub_start,cursor-sub_start);
	if(special_count != arg_count) Log("logging","More arguments passed to loga() than $ characters in the format string");
	LogInternal(str);
}

#endif //DESHI_LOGGING_H
