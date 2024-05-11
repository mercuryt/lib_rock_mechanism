#pragma once

#include "types.h"
#include "../lib/dynamic_bitset.hpp"

#include <vector>

class Area;
class Block;

class OpacityFacade final
{
	Area& m_area;
	sul::dynamic_bitset<> m_fullOpacity;
	sul::dynamic_bitset<> m_floorOpacity;
public:
	OpacityFacade(Area& area);
	void initalize();
	void update(const Block& block);
	[[nodiscard]] bool isOpaque(size_t index) const;
	[[nodiscard]] bool floorIsOpaque(size_t index) const;
	[[nodiscard]] bool hasLineOfSight(const Block& from, const Block& to) const;
	[[nodiscard]] bool canSeeIntoFrom(size_t previousIndex, size_t currentIndex, DistanceInBlocks oldZ, DistanceInBlocks z) const;
};
