#pragma once
#include "Component.h"
#include <time.h>

struct Time : public Component {
	float deltaTime;
	float totalTime;
	uint64 updateCount;

	float physicsTimeStep;
	float physicsDeltaTime;
	float physicsTotalTime;
	float physicsAccumulator;

	bool paused;
	bool frame;

	std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char datentime[30] = {};
	

	Time() {
		deltaTime	= 0.f;
		totalTime	= 0.f;
		updateCount = 0.f;

		physicsTimeStep		= 300.f;
		physicsDeltaTime	= 1.f / physicsTimeStep;
		physicsTotalTime	= 0.f;
		physicsAccumulator	= 0.f;

		paused	= false;
		frame	= false;
	}

	Time(float physicsTicksPerSecond) {
		deltaTime	= 0.f;
		totalTime	= 0.f;
		updateCount = 0.f;

		physicsTimeStep		= physicsTicksPerSecond;
		physicsDeltaTime	= 1.f / physicsTimeStep;
		physicsTotalTime	= 0.f;
		physicsAccumulator		= 0.f;

		paused	= false;
		frame	= false;
	}
};