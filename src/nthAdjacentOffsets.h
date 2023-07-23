#pragma once

#include <unordered_set>
#include <vector>
#include <cassert>

struct XYZ { 
	int32_t x; 
	int32_t y; 
	int32_t z;
	bool operator==(const XYZ& xyz) const
	{
		return xyz.x == x && xyz.y == y && xyz.z == z;
	}
	XYZ operator+(const XYZ& xyz) const
	{
		return XYZ(x + xyz.x, y + xyz.y, z + xyz.z);
	}
};
struct XYZHash
{
	size_t operator()(const XYZ& xyz) const
	{
		assert(abs(xyz.x) < 100);
		assert(abs(xyz.y) < 100);
		assert(abs(xyz.z) < 100);
		return xyz.x * 10000 + xyz.y * 100 + xyz.z;
	}
};
inline std::unordered_set<XYZ, XYZHash> closedList;
inline std::vector<std::vector<XYZ>> cache;

inline constexpr std::array<XYZ, 6> offsets = { XYZ(0,0,-1), XYZ(0,0,1), XYZ(0,-1,0), XYZ(0,1,0), XYZ(-1,0,0), XYZ(1,0,0) };

inline std::vector<XYZ> getNthAdjacentOffsets(uint32_t n)
{
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

	template<class Block>
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
