#pragma once
#ifndef DESHI_TEST_H
#define DESHI_TEST_H

#include "kigu/common.h"

#define TestStartEnvironment(module_name, result_type, result_name, verbose) \
	result_type __deshi_test_result = {};                                    \
	result_type* result_name = &__deshi_test_result;                         \
	b32 __deshi_test_passed = 1;                                             \
	b32 __deshi_test_verbose = verbose;                                      \
	Log("", "Performing tests for ", STRINGIZE(module_name), " module.");

#define TestStartSection(name){                    \
	str8 __deshi_test_section_string = STR8(name); \
	__deshi_test_passed = 1;                       \
	if(__deshi_test_verbose){                      \
		logger_push_indent(1);                     \
		Log("", name, ":");                        \
		logger_push_indent(1);                     \
	}  

#define TestEndSection()                                                     \
	if(__deshi_test_passed){                                                 \
		if(__deshi_test_verbose) logger_pop_indent(2);                       \
		Log("", SuccessFormat("passed"), ": ", __deshi_test_section_string); \
	}else{                                                                   \
		if(__deshi_test_verbose) logger_pop_indent(2);                       \
		Log("", ErrorFormat("failed"), ": ", __deshi_test_section_string);   \
	} }

#define TestStartFunction(name)        \
	if(__deshi_test_verbose) {         \
		Log("", STRINGIZE(name), ":"); \
		logger_push_indent(1);         \
	}
	
#define TestEndFunction(name)  \
	if(__deshi_test_verbose) { \
		logger_pop_indent(1);  \
	}

#define Test(condition)                                                                                     \
	if(!( condition )) {                                                                                    \
		Log("", (__deshi_test_verbose ? "" : "  "), ErrorFormat("failed"), ": ", __FILE__, ":", __LINE__); \
		__deshi_test_passed = 0;                                                                            \
	}

#define TestResult(call,expected)                                                                                                                           \
	call;                                                                                                                                                   \
	if(__deshi_test_result.tag != expected){                                                                                                                \
		Log("",  (__deshi_test_verbose ? "" : "  "), ErrorFormat("failed"), ": ", __FILE__, ":", __LINE__, " with message: ", __deshi_test_result.message); \
		__deshi_test_passed = 0;                                                                                                                            \
	}                                                                                                                                                       \
	__deshi_test_result = {}

#define TestResultSeparate(expected)                                                                                                                        \
	if(__deshi_test_result.tag != expected){                                                                                                                \
		Log("", (__deshi_test_verbose ? "" : "  "), ErrorFormat("failed"), ": ", __FILE__, ":", __LINE__, " with message: ", __deshi_test_result.message); \
		__deshi_test_passed = 0;                                                                                                                            \
	}                                                                                                                                                       \
	__deshi_test_result = {}

#define TestOk(call) TestResult(call, 0)

#define TestReturn(call,expected)                                      \
	if(call != expected){                                              \
		Log("", (__deshi_test_verbose ? "" : "  "), ErrorFormat("failed"), ": ", __FILE__, ":", __LINE__);\
		__deshi_test_passed = 0; \
	}

#define TestReturnTrue(call) TestReturn(call, 1)

#define TestLog(...) if(__deshi_test_verbose) Log("", __VA_ARGS__ )

//TODO(delle) change this to prevent printing
#define TestExpectedLog(...)                       \
	{                                              \
		Logger* logger = logger_expose();          \
		Log("deshi-test","Expected:",__VA_ARGS__); \
		b32 restore = logger->auto_newline;        \
		logger->auto_newline = false;              \
		Log("deshi-test","Actual  :");             \
		logger->auto_newline = restore;            \
	}(void)0

#endif //DESHI_TEST_H
