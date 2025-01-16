/*
 * A cuboid of space where every block has line of sight to every other block.
 * Used to skip line of sight calculations.
 * Stored in Area::m_visionCuboids. Deleted durring DerivedArea::writeStep if m_destroy is true.
 */
#pragma once
#include "cuboid.h"
#include "dataVector.h"

class Area;
class VisionCuboid final
{
public:
	Cuboid m_cuboid;
	VisionCuboidIndex m_index;
	bool m_destroy = false;

	[[nodiscard]] bool canSeeInto(Area& area, const Cuboid& cuboid) const;
	[[nodiscard]] bool canCombineWith(Area& area, const Cuboid& cuboid) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(VisionCuboid, m_cuboid, m_index, m_destroy);
};
class AreaHasVisionCuboids final
{
	DataVector<VisionCuboid, VisionCuboidIndex> m_visionCuboids;
	DataVector<VisionCuboidIndex, BlockIndex> m_blockVisionCuboidIndices;
public:
	void initalize(Area& area);
	void clearDestroyed(Area& area);
	void blockIsNeverOpaque(Area& area, const BlockIndex& block);
	void blockIsSometimesOpaque(Area& area, const BlockIndex& block);
	void blockFloorIsNeverOpaque(Area& area, const BlockIndex& block);
	void blockFloorIsSometimesOpaque(Area& area, const BlockIndex& block);
	void set(Area& area, const BlockIndex& block, VisionCuboid& visionCuboid);
	void unset(const BlockIndex& block);
	void updateBlocks(Area& area, const VisionCuboid& visionCuboid, const VisionCuboidIndex& newIndex);
	void splitAt(Area& area, VisionCuboid& visionCuboid, const BlockIndex& split);
	void splitBelow(Area& area, VisionCuboid& visionCuboid, const BlockIndex& split);
	void extend(Area& area, VisionCuboid& visionCuboid, Cuboid& cuboid);
	void setCuboid(Area& area, VisionCuboid& visionCuboid, Cuboid cuboid);
	VisionCuboid& create(Area& area, Cuboid& cuboid);
	[[nodiscard]] BlockIndex findLowPointForCuboidStartingFromHighPoint(Area& area, const BlockIndex& highest) const;
	[[nodiscard]] VisionCuboid* maybeGetTargetToCombineWith(Area& area, const Cuboid& cuboid);
	[[nodiscard]] VisionCuboid* maybeGetForBlock(const BlockIndex& block);
	[[nodiscard]] VisionCuboidIndex getIndexForBlock(const BlockIndex& block) { return m_blockVisionCuboidIndices[block]; }
	// For testing.
	[[nodiscard]] size_t size() const { return m_visionCuboids.size(); }
	// TODO: blockIndices could be infered.
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasVisionCuboids, m_visionCuboids, m_blockVisionCuboidIndices);
};