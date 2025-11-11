/*
 * A cuboid of space where every point has line of sight to every other point.
 * Used to skip line of sight calculations.
 * Stored in Area::m_visionCuboids. Deleted durring DerivedArea::writeStep if m_destroy is true.
 */
#pragma once
#include "config.h"
#include "../geometry/cuboidMap.h"
#include "../geometry/cuboidSetSIMD.h"
#include "../geometry/cuboidSet.h"
#include "../dataStructures/rtreeData.h"

class Area;
class VisionCuboid;

// To be returned from AreaHasVisionCuboids::query.
// Used by VisionRequest.
class VisionCuboidSetSIMD
{
	CuboidSetSIMD m_cuboidSet;
	Eigen::ArrayX<VisionCuboidIdWidth> m_indices;
public:
	VisionCuboidSetSIMD(uint capacity);
	void insert(const VisionCuboidId& index, const Cuboid& cuboid);
	void maybeInsert(const VisionCuboidId& index, const Cuboid& cuboid);
	void clear();
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] bool contains(const VisionCuboidId& index) const;
	[[nodiscard]] bool contains(const VisionCuboidIdWidth& index) const;
	[[nodiscard]] Cuboid boundry() const { return m_cuboidSet.boundry(); }
};
class AreaHasVisionCuboids final : public CuboidSet
{
	std::vector<VisionCuboidId> m_keys;
	RTreeData<VisionCuboidId> m_pointLookup;
	std::vector<CuboidMap<VisionCuboidId>> m_adjacent;
	Area& m_area;
	VisionCuboidId m_nextKey = VisionCuboidId::create(0);
public:
	void insertOrMerge(const Cuboid& cuboid) override;
	void destroy(const uint& index) override;
	// Override for more efficient lookup of cuboids to destroy.
	void maybeRemove(const Cuboid& cuboid) override;
	void maybeRemove(const CuboidSet& cuboids);
	void onKeySetForPoint(const VisionCuboidId& key, const Point3D& point);
	void sliceBelow(const Cuboid& cuboid);
	void mergeBelow(const Cuboid& cuboid);
	AreaHasVisionCuboids(Area& area);
	void initalize();
	void pointIsTransparent(const Point3D& point);
	void pointIsOpaque(const Point3D& point);
	void pointFloorIsTransparent(const Point3D& point);
	void pointFloorIsOpaque(const Point3D& point);
	void cuboidIsTransparent(const Cuboid& cuboid);
	void cuboidIsOpaque(const Cuboid& cuboid);
	[[nodiscard]] VisionCuboidId getVisionCuboidIndexForPoint(const Point3D& point) const { auto output = m_pointLookup.queryGetOne(point); assert(output.exists()); return output; }
	[[nodiscard]] VisionCuboidId maybeGetVisionCuboidIndexForPoint(const Point3D& point) const { return m_pointLookup.queryGetOne(point); }
	[[nodiscard]] uint getIndexForVisionCuboidId(const VisionCuboidId& index) const;
	[[nodiscard]] Cuboid getCuboidByVisionCuboidId(const VisionCuboidId& index) const;
	[[nodiscard]] Cuboid getCuboidByIndex(const uint& index) const { return m_cuboids[index]; }
	[[nodiscard]] Cuboid getCuboidForPoint(const Point3D& point) const { return getCuboidByVisionCuboidId(getVisionCuboidIndexForPoint(point)); }
	[[nodiscard]] CuboidMap<VisionCuboidId> getAdjacentsForIndex(const uint& index) const { return m_adjacent[index]; }
	[[nodiscard]] CuboidMap<VisionCuboidId> getAdjacentsForVisionCuboid(const VisionCuboidId& index) const { return getAdjacentsForIndex(getIndexForVisionCuboidId(index)); }
	[[nodiscard]] bool canSeeInto(const Cuboid& a, const Cuboid& b) const;
	[[nodiscard]] SmallSet<uint> getMergeCandidates(const Cuboid& cuboid) const;
	[[nodiscard]] VisionCuboidSetSIMD query(const auto& queryShape) const
	{
		VisionCuboidSetSIMD output(16);
		auto eachCuboid = [&](const VisionCuboidId& index, const Cuboid& cuboid) { output.insert(index, cuboid); };
		query(queryShape, eachCuboid);
		return output;
	}
	void query(const auto& queryShape, auto&& action) const
	{
		const Point3D center = queryShape.getCenter();
		VisionCuboidId key = m_pointLookup.queryGetOne(center);
		SmallSet<VisionCuboidId> openList;
		SmallSet<VisionCuboidId> closedList;
		openList.insert(key);
		while(!openList.empty())
		{
			key = openList.back();
			openList.popBack();
			uint index = getIndexForVisionCuboidId(key);
			const Cuboid& cuboid = CuboidSet::m_cuboids[index];
			action(key, cuboid);
			const CuboidMap<VisionCuboidId>& adjacent = m_adjacent[index];
			// TODO: Profile removing this branch.
			if(!adjacent.empty())
				for(const VisionCuboidId& adjacentIndex : adjacent.query(queryShape))
				{
					if(!closedList.contains(adjacentIndex))
					{
						closedList.insert(adjacentIndex);
						openList.insert(adjacentIndex);
					}
				}
		}
	}
	// For testing.
	[[nodiscard]] size_t size() const { return m_keys.size(); }
	void validate() const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasVisionCuboids, m_keys, m_pointLookup, m_adjacent, m_cuboids);
};