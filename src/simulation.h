#pragma once
#include "../lib/BS_thread_pool_light.hpp"
#include "area.h"
#include <list>

namespace simulation
{
	inline uint32_t step;
	inline std::list<Area> areas;
	inline BS::thread_pool_light pool;
	[[maybe_unused]]inline void init(uint32_t s = 0) { step = s; }
	void doStep();
}
