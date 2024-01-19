#include "nthAdjacentOffsets.h"
#include "block.h"
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
		std::unordered_set<XYZ, XYZHash> next;
		for(XYZ xyz : cache.back())
			for(XYZ offset : offsets)
			{
				XYZ adjacent = xyz + offset;
				if(!closedList.contains(adjacent))
				{
					closedList.insert(adjacent);
					next.insert(adjacent);
				}
			}
		cache.emplace_back(next.begin(), next.end());
	}
	return cache.at(n);
}

std::vector<Block*> getNthAdjacentBlocks(Block& center, uint32_t i)
{
	std::vector<Block*> output;
	for(XYZ& offset : getNthAdjacentOffsets(i))
	{
		Block* block = center.offset(offset.x, offset.y, offset.z);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
