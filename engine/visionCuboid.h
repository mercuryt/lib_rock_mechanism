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
	bool m_destroy = false;
public:
	SmallSet<VisionCuboidIndex> m_adjacent;
	Cuboid m_cuboid;
	VisionCuboidIndex m_index;

	VisionCuboid() = default;
	VisionCuboid(const Cuboid& cuboid, const VisionCuboidIndex& index) : m_cuboid(cuboid), m_index(index) { }
	VisionCuboid(const VisionCuboid&) = default;
	VisionCuboid(VisionCuboid&&) noexcept = default;
	VisionCuboid& operator=(const VisionCuboid& other) { m_adjacent = other.m_adjacent; m_cuboid = other.m_cuboid; m_index = other.m_index; m_destroy = other.m_destroy; return *this; }
	VisionCuboid& operator=(VisionCuboid&& other) noexcept { m_adjacent = std::move(other.m_adjacent); m_cuboid = other.m_cuboid; m_index = other.m_index; m_destroy = other.m_destroy; return *this; }
	void setDestroy() { m_destroy = true; }
	[[nodiscard]] bool canSeeInto(Area& area, const Cuboid& cuboid) const;
	[[nodiscard]] bool canCombineWith(Area& area, const Cuboid& cuboid) const;
	// Select a cuboid to steal from this cuboid and merge with the cuboid argument provided.
	[[nodiscard]] Cuboid canStealFrom(Area& area, const Cuboid& cuboid) const;
	[[nodiscard]] bool toDestory() const { return m_destroy; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(VisionCuboid, m_adjacent, m_cuboid, m_index, m_destroy);
};
class AreaHasVisionCuboids final
{
	DataVector<VisionCuboid, VisionCuboidIndex> m_visionCuboids;
	DataVector<VisionCuboidIndex, BlockIndex> m_blockVisionCuboidIndices;
public:
	void initalize(Area& area);
	void clearDestroyed(Area& area);
	// TODO: replace sometimes and never with is / is not. Update with opening / closing of doors and hatches.
	void blockIsTransparent(Area& area, const BlockIndex& block);
	void blockIsOpaque(Area& area, const BlockIndex& block);
	void blockFloorIsTransparent(Area& area, const BlockIndex& block);
	void blockFloorIsOpaque(Area& area, const BlockIndex& block);
	void cuboidIsTransparent(Area& area, const Cuboid& cuboid);
	void cuboidIsOpaque(Area& area, const Cuboid& cuboid);
	void set(Area& area, const BlockIndex& block, VisionCuboid& visionCuboid);
	void unset(const BlockIndex& block);
	void updateBlocks(Area& area, const VisionCuboid& visionCuboid, const VisionCuboidIndex& newIndex);
	void updateBlocks(Area& area, const Cuboid& cuboid, const VisionCuboidIndex& newIndex);
	void splitAt(Area& area, VisionCuboid& visionCuboid, const BlockIndex& split);
	void splitBelow(Area& area, VisionCuboid& visionCuboid, const BlockIndex& split);
	void splitAtCuboid(Area& area, VisionCuboid& visionCuboid, const Cuboid& cuboid);
	void maybeExtend(Area& area, VisionCuboid& visionCuboid);
	void setCuboid(Area& area, VisionCuboid& visionCuboid, const Cuboid& cuboid);
	void createOrExtend(Area& area, const Cuboid& cuboid);
	void setDestroy(VisionCuboid& visionCuboid);
	void validate(const Area& area);
	[[nodiscard]] BlockIndex findLowPointForCuboidStartingFromHighPoint(Area& area, const BlockIndex& highest) const;
	[[nodiscard]] std::pair<VisionCuboid*, Cuboid> maybeGetTargetToCombineWith(Area& area, const Cuboid& cuboid);
	[[nodiscard]] VisionCuboid* maybeGetForBlock(const BlockIndex& block);
	[[nodiscard]] VisionCuboidIndex getIndexForBlock(const BlockIndex& block) { return m_blockVisionCuboidIndices[block]; }
	[[nodiscard]] SmallSet<VisionCuboidIndex> walkAndCollectAdjacentCuboidsInRangeOfPosition(const Area& area, const BlockIndex& location, const DistanceInBlocks& range);
	// For testing.
	[[nodiscard]] size_t size() const { return m_visionCuboids.size(); }
	// TODO: blockIndices could be infered.
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasVisionCuboids, m_visionCuboids, m_blockVisionCuboidIndices);
};