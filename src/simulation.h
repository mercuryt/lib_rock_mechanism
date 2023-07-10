#pragma once
#include "../lib/BS_thread_pool_light.hpp"

class Area;

namespace simulation
{
	uint32_t step;
	std::list<Area> areas;
	BS::thread_pool_light pool;
	void doStep();
}
