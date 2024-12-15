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
	OctTreeNodeIndex m_children;
	Cube m_cube;
	void afterRecord(OctTreeRoot& root, const LocationBucketData& locationData);
	void subdivide(OctTreeRoot& root);
	void record(OctTreeRoot& root, const LocationBucketData& locationData);
public:
	OctTree() = default;
	OctTree(const DistanceInBlocks& halfWidth);
	void record(OctTreeRoot& root, const ActorReference& actor, const Point3D& coordinates, const VisionCuboidId& cuboid, const DistanceInBlocks& visonRangeSquared);
	void erase(OctTreeRoot& root, const ActorReference& actor, const Point3D& coordinates);
	void updateRange(OctTreeRoot& root, const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared);
	void updateVisionCuboid(OctTreeRoot& root, const Point3D& coordinates, const VisionCuboidId& cuboid);
	// TODO: update coordinates.
	template<typename Action>
	void query(const OctTreeRoot& root, const Cube& cube, Action&& action) const;
	template<typename Action>
	void query(const OctTreeRoot& root, const Area& area, const Cuboid& cuboid, Action&& action) const;
	[[nodiscard]] uint8_t getOctant(const Point3D& other);
	friend class OctTreeRoot;
	// For testing.
	[[nodiscard]] uint getCountIncludingChildren(const OctTreeRoot& root) const;
	[[nodiscard]] uint getActorCount() const { return m_data.size(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates) const;
};
class OctTreeRoot
{
	OctTree m_tree;
	DataVector<std::array<OctTree, 8>, OctTreeNodeIndex> m_data;
	DataVector<OctTree*, OctTreeNodeIndex> m_parents;
	bool m_sorted = true;
	OctTreeNodeIndex allocate(OctTree& parent);
	void deallocate(const OctTreeNodeIndex& children);
public:
	OctTreeRoot(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z);
	void record(Area& area, const ActorReference& actor);
	void erase(Area& area, const ActorReference& actor);
	void updateVisionCuboid(const Point3D& coordinates, const VisionCuboidId& cuboid) { m_tree.updateVisionCuboid(*this, coordinates, cuboid); }
	void updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared) { m_tree.updateRange(*this, actor, coordinates, visionRangeSquared); }
	void maybeSort();
	// TODO: update coordinates.
	template<typename Action>
	void query(const Cube& cube, Action&& action) const { m_tree.query(*this, cube, std::move(action)); }
	template<typename Action>
	void query(const Area& area, const Cuboid& cuboid, Action&& action) const { m_tree.query(*this, area, cuboid, std::move(action)); }
	// For testing.
	[[nodiscard]] uint getCount() const { return m_tree.getCountIncludingChildren(*this); }
	[[nodiscard]] uint getActorCount() const { return m_tree.getActorCount(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D coordinates) const { return m_tree.contains(actor, coordinates); }
	friend class OctTree;
};
template<typename Action>
void OctTree::query(const OctTreeRoot& root, const Cube& cube, Action&& action) const
{
	assert(m_cube.intersects(cube));
	if(m_children.empty() || m_cube.isContainedBy(cube))
		m_data.forEach(action);
	else 
		for(const OctTree& child : root.m_data[m_children])
			if(child.m_cube.intersects(cube))
				child.query(root, cube, action);
}
template<typename Action>
void OctTree::query(const OctTreeRoot& root, const Area& area, const Cuboid& cuboid, Action&& action) const
{
	assert(m_cube.intersects(area, cuboid));
	if(m_children.empty() || m_cube.isContainedBy(area, cuboid))
		m_data.forEach(action);
	else 
		for(const OctTree& child : root.m_data[m_children])
			if(child.m_cube.intersects(area, cuboid))
				child.query(root, area, cuboid, action);
}