#include "nthAdjacentOffsets.h"
#include "area/area.h"
#include "types.h"
#include "blocks/blocks.h"
std::vector<Offset3D> getNthAdjacentOffsets(uint32_t n)
{
	if(cache.size() <= n)
	{
		std::lock_guard lock(nthAdjacentMutex);
		while(cache.size() <= n)
		{
			if(cache.empty())
			{
				cache.resize(1);
				cache[0].emplace_back(0,0,0);
			}
			std::vector<Offset3D> next;
			next.reserve(cache.back().size() * 2);
			const auto& lastSet = cache.back();
			for(Offset3D previous : lastSet)
			{
				for(Offset3D offset : adjacentOffsets::direct)
				{
					Offset3D adjacent = previous + offset;
					// Add only if not in previous set or the one before it, if such exists.
					if(
						std::ranges::find(lastSet, adjacent) == lastSet.end() &&
						(
							cache.size() == 1 ||
							std::ranges::find(cache[cache.size() - 2], adjacent) == cache[cache.size() - 2].end()
						)
					)
					{
						next.push_back(adjacent);
					}
				}
			}
			util::removeDuplicates(next);
			cache.emplace_back(next.begin(), next.end());
		}
	}
	return cache[n];
}