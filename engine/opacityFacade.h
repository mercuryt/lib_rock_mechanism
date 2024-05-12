#pragma once

#include "types.h"
#include "../lib/dynamic_bitset.hpp"

#include <vector>

class Area;
class Block;

class OpacityFacade final
{
	Area& m_area;
	std::vector<bool> m_fullOpacity;
	std::vector<bool> m_floorOpacity;
public:
	OpacityFacade(Area& area);
	void initalize();
	void update(const Block& block);
	[[nodiscard]] bool isOpaque(size_t index) const;
	[[nodiscard]] bool floorIsOpaque(size_t index) const;
	[[nodiscard]] bool hasLineOfSight(const Block& from, const Block& to) const;
	[[nodiscard]] bool hasLineOfSight(size_t fromIndex, Point3D fromCoords, size_t toIndex, Point3D toCoords) const;
	[[nodiscard]] bool canSeeIntoFrom(size_t previousIndex, size_t currentIndex, DistanceInBlocks oldZ, DistanceInBlocks z) const;
};
