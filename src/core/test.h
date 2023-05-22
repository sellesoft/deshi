#pragma once
#ifndef DESHI_TEST_H
#define DESHI_TEST_H

#include "kigu/common.h"

#define TestRequires(condition,...) AssertAlways(condition)

//#define Test(condition,...) AssertAlways(condition)



// expects the result handle to be named 'result'
#define Test(call,expected)                                         \
	call;                                                           \
	if(result.tag == expected){                                     \
		Log("", SuccessFormat("succeeded: "), STRINGIZE(call));     \
	}else{                                                          \
		Log("",   ErrorFormat("failed:    "), STRINGIZE(call));     \
	}

#define TestOk(call) Test(call, 0)

#define TestReturn(call,expected)                                   \
	if(call == expected){                                           \
		Log("", SuccessFormat("succeeded: "), STRINGIZE(call));     \
	}else{                                                          \
		Log("",   ErrorFormat("failed:    "), STRINGIZE(call));     \
	}

#define TestReturnTrue(call) TestReturn(call, 1)

#define TestPassed(test) Log("deshi-test","PASSED: " test)
#define TestTodo(test)   Log("deshi-test","TODO:   " test)

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
