#pragma once

#include "dataVector.h"
#include "index.h"
#include "types.h"
#include "visionCuboid.h"

class Area;

class OpacityFacade final
{
	Area& m_area;
	DataBitSet<BlockIndex> m_fullOpacity;
	DataBitSet<BlockIndex> m_floorOpacity;
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
