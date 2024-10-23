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
	void update(const BlockIndex& block);
	void validate() const;
	[[nodiscard]] bool isOpaque(const BlockIndex& index) const;
	[[nodiscard]] bool floorIsOpaque(const BlockIndex& index) const;
	[[nodiscard]] bool hasLineOfSight(const BlockIndex& from, const BlockIndex& to) const;
	[[nodiscard]] bool hasLineOfSight(const BlockIndex& fromIndex, Point3D fromCoords, const BlockIndex& toIndex, Point3D toCoords) const;
	[[nodiscard]] bool canSeeIntoFrom(const BlockIndex& previousIndex, const BlockIndex& currentIndex, const DistanceInBlocks& oldZ, const DistanceInBlocks& z) const;
};
