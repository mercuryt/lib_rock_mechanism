#pragma once

#include "types.h"

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
	void update(const Block& block);
	[[nodiscard]] bool isOpaque(size_t index) const;
	[[nodiscard]] bool floorIsOpaque(size_t index) const;
	[[nodiscard]] bool hasLineOfSight(const Block& from, const Block& to) const;
	[[nodiscard]] bool canSeeIntoFrom(size_t previousIndex, size_t currentIndex, DistanceInBlocks oldZ, DistanceInBlocks z) const;
};
