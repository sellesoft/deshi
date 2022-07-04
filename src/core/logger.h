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

#define ErrorFormat(str)   (VTS_RedFg    str VTS_Default)
#define WarningFormat(str) (VTS_YellowFg str VTS_Default)
#define SuccessFormat(str) (VTS_GreenFg  str VTS_Default)

#define NegativeFormat(str) (VTS_Negative   str VTS_Default)
#define BlackFormat(str)    (VTS_BlackFg    str VTS_Default)          
#define RedFormat(str)      (VTS_RedFg      str VTS_Default)            
#define GreenFormat(str)    (VTS_GreenFg    str VTS_Default)          
#define YellowFormat(str)   (VTS_YellowFg   str VTS_Default)         
#define BlueFormat(str)     (VTS_BlueFg     str VTS_Default)           
#define MagentaFormat(str)  (VTS_MagentaFg  str VTS_Default)        
#define CyanFormat(str)     (VTS_CyanFg     str VTS_Default)           
#define WhiteFormat(str)    (VTS_WhiteFg    str VTS_Default)          
#define RGBFormat(r,g,b,str) (VTS_RGBFg(r,g,b) str VTS_Default)       
#define BlackFormat(str)    (VTS_BlackFg    str VTS_Default)          
#define RedFormat(str)      (VTS_RedFg      str VTS_Default)            
#define GreenFormat(str)    (VTS_GreenFg    str VTS_Default)          
#define YellowFormat(str)   (VTS_YellowFg   str VTS_Default)         
#define BlueFormat(str)     (VTS_BlueFg     str VTS_Default)           
#define MagentaFormat(str)  (VTS_MagentaFg  str VTS_Default)        
#define CyanFormat(str)     (VTS_CyanFg     str VTS_Default)           

#define ErrorFormatDyn(str)   toStr8(VTS_RedFg,    str, VTS_Default).fin
#define WarningFormatDyn(str) toStr8(VTS_YellowFg, str, VTS_Default).fin
#define SuccessFormatDyn(str) toStr8(VTS_GreenFg,  str, VTS_Default).fin

//TODO(sushi) need better way to do this 
#define NegativeFormatDyn(str) toStr8(VTS_Negative,   str, VTS_Default).fin
#define BlackFormatDyn(str)    toStr8(VTS_BlackFg,    str, VTS_Default).fin
#define RedFormatDyn(str)      toStr8(VTS_RedFg,      str, VTS_Default).fin
#define GreenFormatDyn(str)    toStr8(VTS_GreenFg,    str, VTS_Default).fin
#define YellowFormatDyn(str)   toStr8(VTS_YellowFg,   str, VTS_Default).fin
#define BlueFormatDyn(str)     toStr8(VTS_BlueFg,     str, VTS_Default).fin
#define MagentaFormatDyn(str)  toStr8(VTS_MagentaFg,  str, VTS_Default).fin
#define CyanFormatDyn(str)     toStr8(VTS_CyanFg,     str, VTS_Default).fin
#define WhiteFormatDyn(str)    toStr8(VTS_WhiteFg,    str, VTS_Default).fin
#define RGBFormatDyn(r,g,b,str) toStr8(VTS_RGBFg(r,g,b), str, VTS_Default).fin
#define BlackFormatDyn(str)    toStr8(VTS_BlackFg,    str, VTS_Default).fin
#define RedFormatDyn(str)      toStr8(VTS_RedFg,      str, VTS_Default).fin
#define GreenFormatDyn(str)    toStr8(VTS_GreenFg,    str, VTS_Default).fin
#define YellowFormatDyn(str)   toStr8(VTS_YellowFg,   str, VTS_Default).fin
#define BlueFormatDyn(str)     toStr8(VTS_BlueFg,     str, VTS_Default).fin
#define MagentaFormatDyn(str)  toStr8(VTS_MagentaFg,  str, VTS_Default).fin
#define CyanFormatDyn(str)     toStr8(VTS_CyanFg,     str, VTS_Default).fin





#endif //DESHI_LOGGER_H
