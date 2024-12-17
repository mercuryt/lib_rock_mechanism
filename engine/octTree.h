#pragma once
#include "cuboid.h"
#include "types.h"
#include "reference.h"
#include "locationBuckets.h"
#include <memory>
#include <array>

using Allocator = std::pmr::unsynchronized_pool_resource;

class Blocks;
class OctTreeRoot;
class Area;
class OctTree
{
	LocationBucket m_data;
	Cube m_cube;
	OctTreeNodeIndex m_children;
	void afterRecord(OctTreeRoot& root, const LocationBucketData& locationData);
	void subdivide(OctTreeRoot& root);
	void record(OctTreeRoot& root, const LocationBucketData& locationData);
public:
	OctTree(Allocator& allocator) : m_data(allocator) {}
	OctTree(const DistanceInBlocks& halfWidth, Allocator& allocator);
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
	Allocator m_allocator;
	OctTree m_tree;
	DataVector<std::array<OctTree, 8>, OctTreeNodeIndex> m_data;
	DataVector<OctTree*, OctTreeNodeIndex> m_parents;
	uint8_t m_entropy = 0;
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
	void query(const Cube& cube, Action&& action) const
	{
		std::vector<const OctTree*> openList;
		std::vector<const OctTree*> results;
		openList.reserve(m_data.size());
		results.reserve(m_data.size());
		openList.push_back(&m_tree);
		while(!openList.empty())
		{
			const OctTree& candidate = *openList.back();
			openList.pop_back();
			if(!candidate.m_children.exists() || cube.contains(candidate.m_cube))
				results.push_back(&candidate);
			else
				for(const OctTree& child : m_data[candidate.m_children])
				{
					if(child.m_data.empty())
						continue;
					if(cube.contains(child.m_cube))
						results.push_back(&child);
					else if(cube.intersects(child.m_cube))
						openList.push_back(&child);
				}
		}
		for(auto iter = results.begin(); iter != results.end(); ++iter)
		{
			if(iter + 1 != results.end())
				(*(iter + 1))->m_data.prefetch();
			(*iter)->m_data.forEach(action);
		}
	}
	template<typename Action>
	void query(Area& area, const Cuboid& cuboid, Action&& action) const
	{
		std::vector<const OctTree*> openList;
		std::vector<const OctTree*> results;
		openList.reserve(m_data.size());
		results.reserve(m_data.size());
		openList.push_back(&m_tree);
		while(!openList.empty())
		{
			const OctTree& candidate = *openList.back();
			openList.pop_back();
			if(!candidate.m_children.exists() || candidate.m_cube.isContainedBy(area, cuboid))
				results.push_back(&candidate);
			else
				for(const OctTree& child : m_data[candidate.m_children])
				{
					if(child.m_data.empty())
						continue;
					if(child.m_cube.isContainedBy(area, cuboid))
						results.push_back(&child);
					else if(child.m_cube.intersects(area, cuboid))
						openList.push_back(&child);
				}
		}
		for(auto iter = results.begin(); iter != results.end(); ++iter)
		{
			if(iter + 1 != results.end())
				(*(iter + 1))->m_data.prefetch();
			(*iter)->m_data.forEach(action);
		}
	}
	// For testing.
	[[nodiscard]] uint getCount() const { return m_tree.getCountIncludingChildren(*this); }
	[[nodiscard]] uint getActorCount() const { return m_tree.getActorCount(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D coordinates) const { return m_tree.contains(actor, coordinates); }
	friend class OctTree;
};