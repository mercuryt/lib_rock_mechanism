#pragma once

#include "types.h"
#include "../lib/dynamic_bitset.hpp"
#include "visionCuboid.h"

class Area;

class OpacityFacade final
{
	Area& m_area;
	sul::dynamic_bitset<> m_fullOpacity;
	sul::dynamic_bitset<> m_floorOpacity;
public:
	OpacityFacade(Area& area);
	void initalize();
	void update(const BlockIndex block);
	void validate() const;
	[[nodiscard]] bool isOpaque(BlockIndex index) const;
	[[nodiscard]] bool floorIsOpaque(BlockIndex index) const;
	[[nodiscard]] bool hasLineOfSight(BlockIndex from, BlockIndex to) const;
	[[nodiscard]] bool hasLineOfSight(BlockIndex fromIndex, Point3D fromCoords, BlockIndex toIndex, Point3D toCoords) const;
	[[nodiscard]] bool canSeeIntoFrom(BlockIndex previousIndex, BlockIndex currentIndex, DistanceInBlocks oldZ, DistanceInBlocks z) const;
};
