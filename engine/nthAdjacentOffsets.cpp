#include "nthAdjacentOffsets.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"
std::vector<XYZ> getNthAdjacentOffsets(uint32_t n)
{
	assert(n != 0);
	while(cache.size() < n + 1)
	{
		if(cache.empty())
		{
			closedList.emplace(0,0,0);
			cache.resize(n);
			cache[0].emplace_back(0,0,0);
		}
		std::vector<XYZ> next;
		for(XYZ xyz : cache.back())
			for(XYZ offset : offsets)
			{
				XYZ adjacent = xyz + offset;
				if(!closedList.contains(adjacent))
				{
					closedList.insert(adjacent);
					if(std::ranges::find(next, adjacent) == next.end())
						next.push_back(adjacent);
				}
			}
		cache.emplace_back(next.begin(), next.end());
	}
	return cache[n];
}
BlockIndices getNthAdjacentBlocks(Area& area, BlockIndex center, uint32_t i)
{
	BlockIndices output;
	for(XYZ& offset : getNthAdjacentOffsets(i))
	{
		BlockIndex block = area.getBlocks().offset(center, offset.x, offset.y, offset.z);
		if(block.exists())
			output.add(block);
	}
	return output;
}
