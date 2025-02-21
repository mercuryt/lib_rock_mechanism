#include "nthAdjacentOffsets.h"
#include "area/area.h"
#include "types.h"
#include "blocks/blocks.h"
std::vector<Offset3D> getNthAdjacentOffsets(uint32_t n)
{
	assert(n != 0);
	if(cache.size() <= n)
	{
		std::lock_guard lock(nthAdjacentMutex);
		while(cache.size() <= n)
		{
			if(cache.empty())
			{
				closedList.emplace(0,0,0);
				cache.resize(n);
				cache[0].emplace_back(0,0,0);
			}
			std::vector<Offset3D> next;
			for(Offset3D xyz : cache.back())
				for(Offset3D offset : offsets)
				{
					Offset3D adjacent = xyz + offset;
					if(!closedList.contains(adjacent))
					{
						closedList.insert(adjacent);
						if(std::ranges::find(next, adjacent) == next.end())
							next.push_back(adjacent);
					}
				}
			cache.emplace_back(next.begin(), next.end());
		}
	}
	return cache[n];
}
