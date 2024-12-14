#pragma once
#include "cuboid.h"
#include "types.h"
#include "reference.h"
#include "locationBuckets.h"
#include <memory>
#include <array>
class Blocks;
class OctTreeRoot;
class Area;
class OctTree
{
	LocationBucket m_data;
	std::unique_ptr<std::array<OctTree, 8>> m_children;
	Cube m_cube;
	void afterRecord(const LocationBucketData& locationData);
	void subdivide();
	void record(const LocationBucketData& locationData);
public:
	OctTree() = default;
	OctTree(const DistanceInBlocks& halfWidth);
	void record(const ActorReference& actor, const Point3D& coordinates, const VisionCuboidId& cuboid, const DistanceInBlocks& visonRangeSquared);
	void erase(const ActorReference& actor, const Point3D& coordinates);
	void updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared);
	void updateVisionCuboid(const Point3D& coordinates, const VisionCuboidId& cuboid);
	// TODO: update coordinates.
	template<typename Action>
	void query(const Cube& cube, Action&& action) const
	{
		assert(m_cube.intersects(cube));
		if(m_children == nullptr || m_cube.isContainedBy(cube))
			m_data.forEach(action);
		else 
			for(const OctTree& child : *m_children)
				if(child.m_cube.intersects(cube))
					child.query(cube, action);
	}
	template<typename Action>
	void query(Area& area, const Cuboid& cuboid, Action&& action) const
	{
		assert(m_cube.intersects(area, cuboid));
		if(m_children == nullptr || m_cube.isContainedBy(area, cuboid))
			m_data.forEach(action);
		else 
			for(const OctTree& child : *m_children)
				if(child.m_cube.intersects(area, cuboid))
					child.query(area, cuboid, action);
	}
	[[nodiscard]] uint8_t getOctant(const Point3D& other);
	friend class OctTreeRoot;
	// For testing.
	[[nodiscard]] uint getCountIncludingChildren() const;
	[[nodiscard]] uint getActorCount() const { return m_data.size(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates) const;
};
class OctTreeRoot
{
	OctTree m_data;
public:
	OctTreeRoot(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z);
	void record(Area& area, const ActorReference& actor);
	void erase(Area& area, const ActorReference& actor);
	void updateVisionCuboid(const Point3D& coordinates, const VisionCuboidId& cuboid) { m_data.updateVisionCuboid(coordinates, cuboid); }
	void updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared) { m_data.updateRange(actor, coordinates, visionRangeSquared); }
	// TODO: update coordinates.
	template<typename Action>
	void query(const Cube& cube, Action&& action) const { m_data.query(cube, std::move(action)); }
	template<typename Action>
	void query(Area& area, const Cuboid& cuboid, Action&& action) const { m_data.query(area, cuboid, std::move(action)); }
	// For testing.
	[[nodiscard]] uint getCount() const { return m_data.getCountIncludingChildren(); }
	[[nodiscard]] uint getActorCount() const { return m_data.getActorCount(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D coordinates) const { return m_data.contains(actor, coordinates); }
};