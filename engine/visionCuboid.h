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
	//TODO: store these in a data vector indexed by VisionCuboidId.
	std::list<VisionCuboid> m_visionCuboids;
	DataVector<VisionCuboid*, BlockIndex> m_blockVisionCuboids;
	DataVector<VisionCuboidId, BlockIndex> m_blockVisionCuboidIds;
	Area* m_area = nullptr;
	VisionCuboidId m_nextId = VisionCuboidId::create(1);
public:
	void initalize(Area& area);
	void initalizeEmpty(Area& area);
	void clearDestroyed();
	void blockIsNeverOpaque(const BlockIndex& block);
	void blockIsSometimesOpaque(const BlockIndex& block);
	void blockFloorIsNeverOpaque(const BlockIndex& block);
	void blockFloorIsSometimesOpaque(const BlockIndex& block);
	void set(const BlockIndex& block, VisionCuboid& visionCuboid);
	void unset(const BlockIndex& block);
	VisionCuboid& emplace(Cuboid& cuboid);
	[[nodiscard]] VisionCuboid* getTargetToCombineWith(const Cuboid& cuboid);
	[[nodiscard]] VisionCuboidId getIdFor(const BlockIndex& index) const { return m_blockVisionCuboidIds[index]; }
	// For testing.
	[[nodiscard]] size_t size() { return m_visionCuboids.size(); }
	[[nodiscard]] VisionCuboid* getVisionCuboidFor(const BlockIndex& block);
};

class VisionCuboid final
{
	Area& m_area;
public:
	Cuboid m_cuboid;
	const VisionCuboidId m_id = VisionCuboidId::create(0);
	bool m_destroy = false;

	VisionCuboid(Area& area, Cuboid& cuboid, VisionCuboidId id);
	[[nodiscard]] bool canSeeInto(const Cuboid& cuboid) const;
	[[nodiscard]] bool canCombineWith(const Cuboid& cuboid) const;
	void splitAt(const BlockIndex& split);
	void splitBelow(const BlockIndex& split);
	void extend(Cuboid& cuboid);
};