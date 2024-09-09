#pragma once

#include "index.h"
#include "vectorContainers.h"

#include <vector>
#include <array>

class Area;

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
inline std::mutex nthAdjacentMutex;
inline SmallSet<XYZ> closedList;
inline std::vector<std::vector<XYZ>> cache;
inline std::array<XYZ, 6> offsets = { XYZ(0,0,-1), XYZ(0,0,1), XYZ(0,-1,0), XYZ(0,1,0), XYZ(-1,0,0), XYZ(1,0,0) };

std::vector<XYZ> getNthAdjacentOffsets(uint32_t n);
BlockIndices getNthAdjacentBlocks(Area& area, BlockIndex center, uint32_t i);
