#pragma once

#include "dataVector.h"
#include "index.h"
#include "types.h"
#include "vision/visionCuboid.h"

class Area;

class OpacityFacade final
{
	Area& m_area;
	StrongBitSet<BlockIndex> m_fullOpacity;
	StrongBitSet<BlockIndex> m_floorOpacity;
public:
	OpacityFacade(Area& area);
	void initalize();
	void update(const BlockIndex& block);
	void validate() const;
	[[nodiscard]] bool isOpaque(const BlockIndex& index) const;
	[[nodiscard]] bool floorIsOpaque(const BlockIndex& index) const;
	[[nodiscard]] bool hasLineOfSight(const BlockIndex& from, const BlockIndex& to) const;
	[[nodiscard]] bool hasLineOfSight(const BlockIndex& fromIndex, Point3D fromCoords, Point3D toCoords) const;
	[[nodiscard]] bool canSeeIntoFrom(const BlockIndex& previousIndex, const BlockIndex& currentIndex, const DistanceInBlocks& oldZ, const DistanceInBlocks& z) const;
};
