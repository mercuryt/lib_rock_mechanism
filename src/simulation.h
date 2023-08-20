#pragma once
#include "../lib/BS_thread_pool_light.hpp"
#include "area.h"
#include <list>

namespace simulation
{
	inline Step step;
	inline std::list<Area> areas;
	inline BS::thread_pool_light pool;
	void init(Step s = 1);
	void doStep();
}
