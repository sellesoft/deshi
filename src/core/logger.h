#pragma once
#ifndef DESHI_LOGGER_H
#define DESHI_LOGGER_H

#include "kigu/common.h"
#include "kigu/unicode.h"

enum{
	LogType_Normal,
	LogType_Error,
	LogType_Warning,
	LogType_Success,
};

struct File;
struct Logger{
	File* file = 0;
	
#define LOGGER_BUFFER_SIZE 4096
	u8   last_message[LOGGER_BUFFER_SIZE] = {0};
	s64  last_message_length = 0;
	
	b32  mirror_to_file = true;
	b32  mirror_to_stdout = true;
	b32  mirror_to_console = false;
	
	b32  ignore_tags = false;
	b32  auto_newline = true;
	b32  track_caller = false;
	
	s32  indent_level = 0;
	s32  indent_spaces = 2;
};

//comma style:  Log("float: ",12.5f," int: ",0xff)
#define Log(tag,...)       logger_comma_log (str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Normal ,__VA_ARGS__)
#define LogE(tag,...)      logger_comma_log (str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Error  ,__VA_ARGS__)
#define LogW(tag,...)      logger_comma_log (str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Warning,__VA_ARGS__)
#define LogS(tag,...)      logger_comma_log (str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Success,__VA_ARGS__)

//printf style: Logf("float: %f int: %d",12.5f,0xff)
#define Logf(tag,fmt,...)  logger_format_log(str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Normal ,str8_lit(fmt),__VA_ARGS__)
#define LogfE(tag,fmt,...) logger_format_log(str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Error  ,str8_lit(fmt),__VA_ARGS__)
#define LogfW(tag,fmt,...) logger_format_log(str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Warning,str8_lit(fmt),__VA_ARGS__)
#define LogfS(tag,fmt,...) logger_format_log(str8_lit(__FILE__),__LINE__,str8_lit(tag),LogType_Success,str8_lit(fmt),__VA_ARGS__)

//creates a new `Logger.file`, limits the number of logs to `log_count`, and sets `Logger.mirror_to_stdout`/`Logger.mirror_to_console` to `mirror`
void logger_init(u32 log_count = 5, b32 mirror = true);

//flushes the `Logger.file`
void logger_update();

//closes the `Logger.file`
void logger_cleanup();

//returns the internal `Logger` object
FORCE_INLINE Logger* logger_expose();

//increments `Logger.indent_level` by `count`
FORCE_INLINE void logger_push_indent(s32 count = 1);

//decrements `Logger.indent_level` by `count`, if `count` is negative, set `Logger.indent_level` to zero
FORCE_INLINE void logger_pop_indent(s32 count = 1);

//returns a `str8` of the last message logged
FORCE_INLINE str8 logger_last_message();

//logs a message in printf style using `fmt`
void logger_format_log(str8 caller_file, upt caller_line, str8 tag, Type log_type, str8 fmt, ...);

void logger_comma_log_internal(str8 caller_file, upt caller_line, str8 tag, Type log_type, string* arr, u32 arr_count);

//logs a message in comma style using `args`
template<typename... T> inline void
logger_comma_log(str8 caller_file, upt caller_line, str8 tag, Type log_type, T... args){
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(args)...}; //TODO use temp allocation
	logger_comma_log_internal(caller_file, caller_line, tag, log_type, arr, arg_count);
}

#endif //DESHI_LOGGER_H
