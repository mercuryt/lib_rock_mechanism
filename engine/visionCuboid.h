/*
 * A cuboid of space where every block has line of sight to every other block.
 * Used to skip line of sight calculations.
 * Stored in Area::m_visionCuboids. Deleted durring DerivedArea::writeStep if m_destroy is true.
 */
#pragma once
#include <list>
#include <vector>
#include "cuboid.h"
#include "dataVector.h"
#include "types.h"

class Area;
class VisionCuboid;

class AreaHasVisionCuboids final
{
	std::list<VisionCuboid> m_visionCuboids;
	DataVector<VisionCuboid*, BlockIndex> m_blockVisionCuboids;
	DataVector<VisionCuboidId, BlockIndex> m_blockVisionCuboidIds;
	Area* m_area;
	VisionCuboidId m_nextId = 1;
public:
	void initalize(Area& area);
	void clearDestroyed();
	void blockIsNeverOpaque(BlockIndex block);
	void blockIsSometimesOpaque(BlockIndex block);
	void blockFloorIsNeverOpaque(BlockIndex block);
	void blockFloorIsSometimesOpaque(BlockIndex block);
	void set(BlockIndex block, VisionCuboid& visionCuboid);
	void unset(BlockIndex block);
	VisionCuboid& emplace(Cuboid& cuboid);
	[[nodiscard]] VisionCuboid* getTargetToCombineWith(const Cuboid& cuboid);
	[[nodiscard]] VisionCuboidId getIdFor(BlockIndex index) const { return m_blockVisionCuboidIds.at(index); }
	// For testing.
	[[nodiscard]] size_t size() { return m_visionCuboids.size(); }
	[[nodiscard]] VisionCuboid* getVisionCuboidFor(BlockIndex block);
};

class VisionCuboid final
{
	Area& m_area;
public:
	Cuboid m_cuboid;
	const VisionCuboidId m_id = 0;
	bool m_destroy = false;

	VisionCuboid(Area& area, Cuboid& cuboid, VisionCuboidId id);
	[[nodiscard]] bool canSeeInto(const Cuboid& cuboid) const;
	[[nodiscard]] bool canCombineWith(const Cuboid& cuboid) const;
	void splitAt(BlockIndex split);
	void splitBelow(BlockIndex split);
	void extend(Cuboid& cuboid);
};
