#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"
#include "../lib/BS_thread_pool_light.hpp"

class Area;

namespace simulation
{
	uint32_t step;
	std::list<Area> areas;
	BS::thread_pool_light pool;
	EventSchedule eventSchedule;
	ThreadedTaskEngine threadedTaskEngine;
	void doStep();
}
