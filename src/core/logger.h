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
Logger* logger_expose();

//increments `Logger.indent_level` by `count`
inline void logger_push_indent(s32 count = 1);

//decrements `Logger.indent_level` by `count`, if `count` is negative, set `Logger.indent_level` to zero
inline void logger_pop_indent(s32 count = 1);

//returns a `str8` of the last message logged
str8 logger_last_message();

//logs a message in printf style using `fmt`
void logger_format_log(str8 caller_file, upt caller_line, str8 tag, Type log_type, str8 fmt, ...);

void logger_comma_log_internal(str8 caller_file, upt caller_line, str8 tag, Type log_type, string* arr, u32 arr_count);

//logs a message in comma style using `args`
template<typename... T> inline void
logger_comma_log(str8 caller_file, upt caller_line, str8 tag, Type log_type, T... args){
	StaticAssert(sizeof...(T) > 0, "A call to Log() was empty or did not specify a tag.");
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(args)...}; //TODO use temp allocation
	logger_comma_log_internal(caller_file, caller_line, tag, log_type, arr, arg_count);
}

#define VTS_Default         "\x1b[0m"
#define VTS_Bold            "\x1b[1m"
#define VTS_NoBold          "\x1b[22m"
#define VTS_Underline       "\x1b[4m"
#define VTS_NoUnderline     "\x1b[24m"
#define VTS_Negative        "\x1b[7m"
#define VTS_Positive        "\x1b[27m"
#define VTS_BlackFg         "\x1b[30m"
#define VTS_RedFg           "\x1b[31m"
#define VTS_GreenFg         "\x1b[32m"
#define VTS_YellowFg        "\x1b[33m"
#define VTS_BlueFg          "\x1b[34m"
#define VTS_MagentaFg       "\x1b[35m"
#define VTS_CyanFg          "\x1b[36m"
#define VTS_WhiteFg         "\x1b[37m"
#define VTS_ExtendedFg      "\x1b[38m"
#define VTS_DefaultFg       "\x1b[39m"
#define VTS_BlackBg         "\x1b[40m"
#define VTS_RedBg           "\x1b[41m"
#define VTS_GreenBg         "\x1b[42m"
#define VTS_YellowBg        "\x1b[43m"
#define VTS_BlueBg          "\x1b[44m"
#define VTS_MagentaBg       "\x1b[45m"
#define VTS_CyanBg          "\x1b[46m"
#define VTS_WhiteBg         "\x1b[47m"
#define VTS_RGBBg(r,g,b)      "\x1b[48;2;" STRINGIZE(r) ";" STRINGIZE(g) ";" STRINGIZE(b) "m"
#define VTS_DefaultBg       "\x1b[49m"
#define VTS_BrightBlackFg   "\x1b[90m"
#define VTS_BrightRedFg     "\x1b[91m"
#define VTS_BrightGreenFg   "\x1b[92m"
#define VTS_BrightYellowFg  "\x1b[93m"
#define VTS_BrightBlueFg    "\x1b[94m"
#define VTS_BrightMagentaFg "\x1b[95m"
#define VTS_BrightCyanFg    "\x1b[96m"
#define VTS_BrightWhiteFg   "\x1b[97m"
#define VTS_BrightBlackBg   "\x1b[100m"
#define VTS_BrightRedBg     "\x1b[101m"
#define VTS_BrightGreenBg   "\x1b[102m"
#define VTS_BrightYellowBg  "\x1b[103m"
#define VTS_BrightBlueBg    "\x1b[104m"
#define VTS_BrightMagentaBg "\x1b[105m"
#define VTS_BrightCyanBg    "\x1b[106m"
#define VTS_BrightWhiteBg   "\x1b[107m"

#define ErrorFormat(str)   VTS_RedFg    str VTS_Default
#define WarningFormat(str) VTS_YellowFg str VTS_Default
#define SuccessFormat(str) VTS_GreenFg  str VTS_Default

#define NegativeFormat(str) VTS_Negative   str VTS_Default
#define BlackFormat(str)    VTS_BlackFg    str VTS_Default
#define RedFormat(str)      VTS_RedFg      str VTS_Default
#define GreenFormat(str)    VTS_GreenFg    str VTS_Default
#define YellowFormat(str)   VTS_YellowFg   str VTS_Default
#define BlueFormat(str)     VTS_BlueFg     str VTS_Default
#define MagentaFormat(str)  VTS_MagentaFg  str VTS_Default
#define CyanFormat(str)     VTS_CyanFg     str VTS_Default
#define WhiteFormat(str)    VTS_WhiteFg    str VTS_Default
#define RGBFormat(r,g,b,str) VTS_RGBFg(r,g,b) str VTS_Default
#define BrightBlackFormat(str)    VTS_BrightBlackFg    str VTS_Default
#define BrightRedFormat(str)      VTS_BrightRedFg      str VTS_Default
#define BrightGreenFormat(str)    VTS_BrightGreenFg    str VTS_Default
#define BrightYellowFormat(str)   VTS_BrightYellowFg   str VTS_Default
#define BrightBlueFormat(str)     VTS_BrightBlueFg     str VTS_Default
#define BrightMagentaFormat(str)  VTS_BrightMagentaFg  str VTS_Default
#define BrightCyanFormat(str)     VTS_BrightCyanFg     str VTS_Default

#define ErrorFormatComma(...)   VTS_RedFg,    __VA_ARGS__, VTS_Default
#define WarningFormatComma(...) VTS_YellowFg, __VA_ARGS__, VTS_Default
#define SuccessFormatComma(...) VTS_GreenFg,  __VA_ARGS__, VTS_Default

#define NegativeFormatComma(...) VTS_Negative,   __VA_ARGS__, VTS_Default
#define BlackFormatComma(...)    VTS_BlackFg,    __VA_ARGS__, VTS_Default
#define RedFormatComma(...)      VTS_RedFg,      __VA_ARGS__, VTS_Default
#define GreenFormatComma(...)    VTS_GreenFg,    __VA_ARGS__, VTS_Default
#define YellowFormatComma(...)   VTS_YellowFg,   __VA_ARGS__, VTS_Default
#define BlueFormatComma(...)     VTS_BlueFg,     __VA_ARGS__, VTS_Default
#define MagentaFormatComma(...)  VTS_MagentaFg,  __VA_ARGS__, VTS_Default
#define CyanFormatComma(...)     VTS_CyanFg,     __VA_ARGS__, VTS_Default
#define WhiteFormatComma(...)    VTS_WhiteFg,    __VA_ARGS__, VTS_Default
#define RGBFormatComma(r,g,b,...) VTS_RGBFg(r,g,b), __VA_ARGS__, VTS_Default
#define BrightBlackFormatComma(...)    VTS_BrightBlackFg,    __VA_ARGS__, VTS_Default
#define BrightRedFormatComma(...)      VTS_BrightRedFg,      __VA_ARGS__, VTS_Default
#define BrightGreenFormatComma(...)    VTS_BrightGreenFg,    __VA_ARGS__, VTS_Default
#define BrightYellowFormatComma(...)   VTS_BrightYellowFg,   __VA_ARGS__, VTS_Default
#define BrightBlueFormatComma(...)     VTS_BrightBlueFg,     __VA_ARGS__, VTS_Default
#define BrightMagentaFormatComma(...)  VTS_BrightMagentaFg,  __VA_ARGS__, VTS_Default
#define BrightCyanFormatComma(...)     VTS_BrightCyanFg,     __VA_ARGS__, VTS_Default

#define ErrorFormatDyn(str)   toStr8(VTS_RedFg,    str, VTS_Default)
#define WarningFormatDyn(str) toStr8(VTS_YellowFg, str, VTS_Default)
#define SuccessFormatDyn(str) toStr8(VTS_GreenFg,  str, VTS_Default)

//TODO(sushi) need better way to do this
#define NegativeFormatDyn(str) toStr8(VTS_Negative,   str, VTS_Default)
#define BlackFormatDyn(str)    toStr8(VTS_BlackFg,    str, VTS_Default)
#define RedFormatDyn(str)      toStr8(VTS_RedFg,      str, VTS_Default)
#define GreenFormatDyn(str)    toStr8(VTS_GreenFg,    str, VTS_Default)
#define YellowFormatDyn(str)   toStr8(VTS_YellowFg,   str, VTS_Default)
#define BlueFormatDyn(str)     toStr8(VTS_BlueFg,     str, VTS_Default)
#define MagentaFormatDyn(str)  toStr8(VTS_MagentaFg,  str, VTS_Default)
#define CyanFormatDyn(str)     toStr8(VTS_CyanFg,     str, VTS_Default)
#define WhiteFormatDyn(str)    toStr8(VTS_WhiteFg,    str, VTS_Default)
#define RGBFormatDyn(r,g,b,str) toStr8(VTS_RGBFg(r,g,b), str, VTS_Default)
#define BlackFormatDyn(str)    toStr8(VTS_BlackFg,    str, VTS_Default)
#define RedFormatDyn(str)      toStr8(VTS_RedFg,      str, VTS_Default)
#define GreenFormatDyn(str)    toStr8(VTS_GreenFg,    str, VTS_Default)
#define YellowFormatDyn(str)   toStr8(VTS_YellowFg,   str, VTS_Default)
#define BlueFormatDyn(str)     toStr8(VTS_BlueFg,     str, VTS_Default)
#define MagentaFormatDyn(str)  toStr8(VTS_MagentaFg,  str, VTS_Default)
#define CyanFormatDyn(str)     toStr8(VTS_CyanFg,     str, VTS_Default)

#endif //DESHI_LOGGER_H

#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_LOGGER_IMPL)
#define DESHI_LOGGER_IMPL

//NOTE(delle) these are used in logger_init() for setting Win32 stdout mode to UTF16
#include <fcntl.h>
//#include <ctime>
#include <clocale>
#ifdef DESHI_LINUX
#  include "wctype.h"
#endif 
#include "file.h" 


//NOTE(delle) customize this as you see fit locally, but it should be 1 on commit
#if DESHI_INTERNAL
#  define LOGGER_ASSERT_ON_ERROR 1
#else
#  define LOGGER_ASSERT_ON_ERROR 0
#endif //#if DESHI_INTERNAL

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @internal
local Logger logger;

int
logger_message_prefix(int cursor, str8 caller_file, upt caller_line, str8 tag, Type log_type){DPZoneScoped;
#if LOGGER_ASSERT_ON_ERROR
	if(log_type == LogType_Error){
		Assert(!"assert on error");
	}
#endif //#if LOGGER_ASSERT_ON_ERROR
	
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
								 "%s(%zu): ", (char*)caller_file.str, caller_line);
	}
	
	return cursor;
}

int
logger_message_postfix(int cursor, str8 tag, Type log_type){DPZoneScoped;
	//write to stdout (convert to wide characters b/c MSVC/Windows requires it)
	if(logger.mirror_to_stdout){
#if DESHI_WINDOWS
		persist wchar_t logger_conversion_buffer[LOGGER_BUFFER_SIZE] = {0};
		logger.last_message[cursor] = '\0';
		mbstowcs(logger_conversion_buffer, (char*)logger.last_message, LOGGER_BUFFER_SIZE-1);
		fputws(logger_conversion_buffer, stdout);
		if(logger.auto_newline) fputwc('\n', stdout);
#else
		logger.last_message[cursor] = '\0';
		if(logger.auto_newline){
			printf("%s\n", logger.last_message);
		}else{
			printf("%s", logger.last_message);
			fflush(stdout);
		}
#endif 
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
		u32 message_offset = 0;
		Type type;
		switch(log_type){
			case LogType_Normal: { type = ConsoleChunkType_Normal;  message_offset = (tag.count?tag.count+ 3:0); } break;
			case LogType_Error:  { type = ConsoleChunkType_Error;   message_offset = tag.count+18; } break;
			case LogType_Warning:{ type = ConsoleChunkType_Warning; message_offset = tag.count+20; } break;
			case LogType_Success:{ type = ConsoleChunkType_Success; message_offset = tag.count+20; } break;
		}
		console_parse_message({logger.last_message+message_offset,logger.last_message_length-message_offset},tag,type,1,log_file_offset+message_offset);
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
	File* log_files = file_search_directory(str8_lit("data/logs/"));
	if(!log_files) return;

	//rename previous log.txt
	forX_array(file, log_files){
		if(str8_equal(file->name, str8_lit("log.txt"))){
			int len = stbsp_snprintf((char*)path_buffer, ArrayCount(path_buffer), "data/logs/log_%lld.txt", file->last_write_time);
			file_rename(file->path, (str8{path_buffer, (s64)len}));
			file->path = str8{path_buffer, (s64)len}; //NOTE(delle) correcting path on temp memory so it's valid for deletion
		}
	}
	
	//delete all but last 'log_count' files in logs directory
	if(array_count(log_files) > log_count){
		//sort logs ascending based on last write time
		b32 swapped = false;
		forX(i,log_count){
			swapped = false;
			forX(j,array_count(log_files)-1-i){
				if(log_files[j].last_write_time > log_files[j+1].last_write_time){
					Swap(log_files[j], log_files[j+1]);
					swapped = true;
				}
			}
			if(!swapped) break;
		}
		
		//delete logs
		forI((array_count(log_files)-log_count)+1) file_delete(log_files[i].path, FileDeleteFlags_File);
	}
	
	array_deinit(log_files);

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
	
	fflush(stdout); 
#if DESHI_WINDOWS
	setlocale(LC_ALL, ".utf8");
	_setmode(_fileno(stdout), _O_U16TEXT); //NOTE(delle) enables Unicode printing to stdout on Windows
#elif DESHI_LINUX
	// TODO(sushi) confirm that this is consistent among linux distributions, this is just what it is on mine
	//             use 'locale -a' on cli to see a list of strings you can use here
	setlocale(LC_ALL, "en_US.utf8");
#endif
	
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
	fflush(stdout);
#if DESHI_WINDOWS
	_setmode(_fileno(stdout), _O_TEXT); //NOTE(delle) disable Unicode printing to stdout on Windows
#endif
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

int test_add(int a, int b){
	return a+b;
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

#endif
