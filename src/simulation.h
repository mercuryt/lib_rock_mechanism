#pragma once
#include "area.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "../lib/BS_thread_pool_light.hpp"

class Simulation
{
	uint32_t m_step;
	std::list<Area> m_areas;
	BS::thread_pool_light m_pool;
	EventSchedule m_eventSchedule;
	ThreadedTaskEngine m_threadedTaskEngine;
public:
	Simulation() { m_threadedTaskEngine.setPool(m_pool); }
	void step();
};
