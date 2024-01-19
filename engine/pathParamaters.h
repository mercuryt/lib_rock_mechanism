#pragma once

#include <functional>
#include <cstdint>
#include <cassert>
class Block;
struct PathParamaters
{
	std::function<bool(const Block&)> predicate;
	Block* huristicDestination = nullptr;
	bool adjacent = false;
	bool reserveDestination = true;
	bool reserveTarget = false;
	bool detour = false;
	bool unreservedTarget = true;
	bool unreservedDestination = true;
	uint32_t maxRange = UINT32_MAX;
	void normalize()
	{
		if(reserveTarget)
		{
			assert(adjacent);
			unreservedTarget = true;
		}
		if(reserveDestination)
			unreservedDestination = true;;
		assert(maxRange != 0);
	}
};
