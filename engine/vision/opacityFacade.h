#pragma once

#include "dataStructures/strongVector.h"
#include "index.h"
#include "types.h"
#include "vision/visionCuboid.h"

class Area;

class OpacityFacade final
{
	Area& m_area;
	StrongBitSet<BlockIndexChunked> m_fullOpacity;
	StrongBitSet<BlockIndexChunked> m_floorOpacity;
public:
	OpacityFacade(Area& area);
	void initalize();
	void update(const BlockIndex& block);
	void validate() const;
	[[nodiscard]] bool isOpaque(const BlockIndexChunked& index) const;
	[[nodiscard]] bool floorIsOpaque(const BlockIndexChunked& index) const;
	[[nodiscard]] bool hasLineOfSight(const BlockIndex& from, const BlockIndex& to) const;
	[[nodiscard]] bool hasLineOfSight(const Point3D& fromCoords, const Point3D& toCoords) const;
	[[nodiscard]] bool canSeeIntoFrom(const BlockIndexChunked& previousIndex, const BlockIndexChunked& currentIndex, const DistanceInBlocks& oldZ, const DistanceInBlocks& z) const;
};
