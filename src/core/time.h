#pragma once
#ifndef DESHI_TIME_H
#define DESHI_TIME_H

#include "../utils/defines.h"

#include <chrono>
#include <ctime>
#include <string>

#define TIMER_START(name) std::chrono::time_point<std::chrono::high_resolution_clock> name = std::chrono::high_resolution_clock::now();
#define TIMER_RESET(name) name = std::chrono::high_resolution_clock::now();
#define TIMER_END(name) std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - name).count()

struct Time{
	f32 prevDeltaTime;
	f32 deltaTime;
	f64 totalTime;
	u64 updateCount;
	
	f32 fixedTimeStep;
	f32 fixedDeltaTime;
	f64 fixedTotalTime;
	u64 fixedUpdateCount;
	f32 fixedAccumulator;
	
	bool paused, frame;
	
	std::chrono::time_point<std::chrono::system_clock> tp1, tp2;
	
	void Init(float fixedUpdatesPerSecond);
	void Update();
	
	std::string FormatDateTime(std::string format);
};

inline void Time::Init(float fixedUpdatesPerSecond){
	prevDeltaTime = 0;
	deltaTime     = 0;
	totalTime     = 0;
	updateCount   = 0;
	
	fixedTimeStep    = fixedUpdatesPerSecond;
	fixedDeltaTime   = 1.f / fixedUpdatesPerSecond;
	fixedTotalTime   = 0;
	fixedUpdateCount = 0;
	fixedAccumulator = 0;
	
	paused = false;
	frame  = false;
	
	tp1 = std::chrono::system_clock::now();
	tp2 = std::chrono::system_clock::now();
}

inline void Time::Update(){
	tp2 = std::chrono::system_clock::now();
	std::chrono::duration<float> elapsedTime = tp2 - tp1;
	tp1 = tp2;
	
	prevDeltaTime = deltaTime;
	deltaTime = elapsedTime.count();
	
	if(!paused){
		totalTime += deltaTime;
		++updateCount;
		fixedAccumulator += deltaTime;
	}else if(frame){
		totalTime += deltaTime;
		++updateCount;
		fixedAccumulator += deltaTime;
		frame = false;
	}else{
		deltaTime = 0;
	}
}

//{y}:year, {M}:month, {d}:day, {h}:hour, {m}:minute, {s}:second, {w}:weekday
inline std::string Time::FormatDateTime(std::string fmt){
	std::time_t now = time(0);
	std::tm* ltm = localtime(&now);
	std::string weekday;
	switch (ltm->tm_wday) {
		case 0: weekday = "Mon"; break;
		case 1: weekday = "Tue"; break;
		case 2: weekday = "Wed"; break;
		case 3: weekday = "Thu"; break;
		case 4: weekday = "Fri"; break;
		case 5: weekday = "Sat"; break;
		case 6: weekday = "Sun"; break;
	}
	
	std::string out = ""; out.reserve(256);
	for_n(i, fmt.size()){
		if(fmt[i] == '{'){
			switch(fmt[i+1]){
				case('y'):{
					out.append(std::to_string(ltm->tm_year + 1900));
				}i+=2;continue;
				case('M'):{
					out.append((ltm->tm_mon + 1 > 9) ? std::to_string(ltm->tm_mon + 1) : std::string("0") + std::to_string(ltm->tm_mon + 1));
				}i+=2;continue;
				case('d'):{
					out.append((ltm->tm_mday > 9) ? std::to_string(ltm->tm_mday) : std::string("0") + std::to_string(ltm->tm_mday));
				}i+=2;continue;
				case('h'):{
					out.append((ltm->tm_hour > 9) ? std::to_string(ltm->tm_hour) : std::string("0") + std::to_string(ltm->tm_hour));
				}i+=2;continue;
				case('m'):{
					out.append((ltm->tm_min > 9) ? std::to_string(ltm->tm_min) : std::string("0") + std::to_string(ltm->tm_min));
				}i+=2;continue;
				case('s'):{
					out.append((ltm->tm_sec > 9) ? std::to_string(ltm->tm_sec) : std::string("0") + std::to_string(ltm->tm_sec));
				}i+=2;continue;
				case('w'):{
					out.append(weekday);
				}i+=2;continue;
			}
		}
		out.push_back(fmt[i]);
	}
	
	out.shrink_to_fit(); return out;
}

#endif //DESHI_TIME_H