/*
 * A cuboid of space where every block has line of sight to every other block.
 * Used to skip line of sight calculations.
 * Stored in Area::m_visionCuboids. Deleted durring DerivedArea::writeStep if m_destroy is true.
 */
#pragma once
#include <list>
#include <vector>
#include "cuboid.h"
#include "types.h"

class Area;
class VisionCuboid;

class AreaHasVisionCuboids final
{
	std::list<VisionCuboid> m_visionCuboids;
	std::vector<VisionCuboid*> m_blockVisionCuboids;
	std::vector<VisionCuboidId> m_blockVisionCuboidIds;
	VisionCuboidId m_nextId = 1;
public:
	void initalize(Area& area);
	void clearDestroyed();
	void blockIsNeverOpaque(Block& block);
	void blockIsSometimesOpaque(Block& block);
	void blockFloorIsNeverOpaque(Block& block);
	void blockFloorIsSometimesOpaque(Block& block);
	void set(Block& block, VisionCuboid& visionCuboid);
	void unset(Block& block);
	VisionCuboid& emplace(Cuboid& cuboid);
	[[nodiscard]] VisionCuboid* getTargetToCombineWith(const Cuboid& cuboid);
	[[nodiscard]] VisionCuboidId getIdFor(BlockIndex index) const { return m_blockVisionCuboidIds[index]; }
	// For testing.
	[[nodiscard]] size_t size() { return m_visionCuboids.size(); }
	[[nodiscard]] VisionCuboid* getVisionCuboidFor(const Block& block);
};

class VisionCuboid final
{
public:
	Cuboid m_cuboid;
	const VisionCuboidId m_id = 0;
	bool m_destroy = false;

	VisionCuboid(Cuboid& cuboid, VisionCuboidId id);
	[[nodiscard]] bool canSeeInto(const Cuboid& cuboid) const;
	[[nodiscard]] bool canCombineWith(const Cuboid& cuboid) const;
	void splitAt(Block& split);
	void splitBelow(Block& split);
	void extend(Cuboid& cuboid);
};
