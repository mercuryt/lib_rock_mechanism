#pragma once

#include <unordered_set>
#include <vector>
#include <cassert>
#include <cstdint>

class Block;

struct XYZ { 
	int32_t x; 
	int32_t y; 
	int32_t z;
	XYZ(int32_t ax, int32_t ay, int32_t az) : x(ax), y(ay), z(az) { }
	bool operator==(const XYZ& xyz) const
	{
		return xyz.x == x && xyz.y == y && xyz.z == z;
	}
	XYZ operator+(const XYZ& other) const
	{
		return XYZ(x + other.x, y + other.y, z + other.z);
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
//TODO: make constexpr?
inline static std::array<XYZ, 6> offsets = { XYZ(0,0,-1), XYZ(0,0,1), XYZ(0,-1,0), XYZ(0,1,0), XYZ(-1,0,0), XYZ(1,0,0) };

std::vector<XYZ> getNthAdjacentOffsets(uint32_t n);
std::vector<Block*> getNthAdjacentBlocks(Block& center, uint32_t i);
