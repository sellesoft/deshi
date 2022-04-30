#pragma once
#ifndef DESHI_TIME_H
#define DESHI_TIME_H

#include "kigu/common.h"

//NOTE(delle) real-world times should be in 64bit unix format
#define WindowsTimeToUnixTime(x) ((u64(x) / 10000000LL) - 11644473600LL)

typedef s64 Stopwatch;

struct Time{
	f64 prevDeltaTime;
	f64 deltaTime;
	f64 totalTime;
	u64 frame;
	f64 timeTime, windowTime, inputTime, consoleTime, renderTime, frameTime;
	Stopwatch stopwatch;
};

//global time pointer
extern Time* g_time;
#define DeshTime g_time
#define DeshTotalTime g_time->totalTime

//Returns a `Stopwatch` representing the start time
inline Stopwatch start_stopwatch();

//Returns the time since `watch` was started in milliseconds
inline f64 peek_stopwatch(Stopwatch watch);

//Resets the `watch` and returns the time since it was started in milliseconds
inline f64 reset_stopwatch(Stopwatch* watch){
	f64 result = peek_stopwatch(*watch);
	*watch = start_stopwatch();
	return result;
}

//TODO(delle) countdown timers with callback functions on finish
//TODO(delle) sleep functions

#endif //DESHI_TIME_H