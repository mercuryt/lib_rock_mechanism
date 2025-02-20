#pragma once
#include "dataVector.h"
#include "geometry/cuboidSetSIMD.h"
#include "geometry/point3D.h"
#include "locationBuckets.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../lib/Eigen/Dense"
#pragma GCC diagnostic pop

#include <memory>

using Allocator = std::pmr::unsynchronized_pool_resource;

class Area;
class VisionCuboidId;
class DistanceInBlocks;

class OctTreeIndex : public StrongInteger<OctTreeIndex, uint16_t>
{
public:
	OctTreeIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const OctTreeIndex& index) const { return index.get(); } };
};

struct OctTreeNode
{
	LocationBucket contents;
	CuboidArray<8> cuboids;
	std::array<OctTreeIndex, 8> children;
	Cuboid cuboid;
	Point3D center;
	OctTreeIndex parent;
	// Do not initalize center here, leaving it blank is the symbol that this node has no children.
	// Center will be set along with cuboids and children during split.
	OctTreeNode() = default;
	OctTreeNode(const OctTreeIndex& p, const Cuboid& c) : cuboid(c), parent(p) { }
	OctTreeNode(const OctTreeNode& other) = default;
	OctTreeNode(OctTreeNode&& other) noexcept = default;
	void operator=(const OctTreeNode& other) {contents = other.contents; cuboids = other.cuboids; children = other.children; cuboid = other.cuboid; center = other.center; parent = other.parent; }
	void operator=(OctTreeNode&& other) noexcept {contents = std::move(other.contents); cuboids = other.cuboids; children = other.children; cuboid = other.cuboid; center = other.center; parent = other.parent; }
	[[nodiscard]] bool hasChildren() const { return !center.empty(); }
	[[nodiscard]] bool shouldSplit() const { return !hasChildren() && size() >= Config::minimumOccupantsForOctTreeToSplit && cuboid.dimensionForFacing(Facing6::Above) >= Config::minimumSizeForOctTreeToSplit; }
	[[nodiscard]] bool shouldMerge() const { return hasChildren() && size() <= Config::maximumOccupantsForOctTreeToMerge; }
	[[nodiscard]] uint size() const { return contents.size(); }
};

class OctTree
{
	Allocator m_allocator;
	StrongVector<OctTreeNode, OctTreeIndex> m_nodes;
public:
	OctTree(const Cuboid& cuboid);
	void record(Area& area, const ActorReference& actor);
	void erase(Area& area, const ActorReference& actor);
	void updateVisionCuboid(const Point3D& coordinates, const VisionCuboidId& cuboid);
	void updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared);
	void maybeSort();
	void split(const OctTreeIndex& node);
	void collapse(const OctTreeIndex& node);
	[[nodiscard]] uint getCount() const { return m_nodes.size(); }
	[[nodiscard]] uint getActorCount() const;
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates);
	[[nodiscard]] static int8_t getOctant(const Point3D& center, const Point3D& coordinates);
	[[nodiscard]] static CuboidArray<8> subdivide(const Cuboid& cuboid);
	template<class Action>
	void query(const auto& queryShape, const VisionCuboidSetSIMD* visionCuboids, Action&& action)
	{
		const OctTreeIndex topLevelIndex = OctTreeIndex::create(0);
		const OctTreeNode topLevelNode = m_nodes[topLevelIndex];
		Cuboid visionCuboidsBoundry;
		if(visionCuboids != nullptr)
			visionCuboidsBoundry = visionCuboids->boundry();
		if(!topLevelNode.hasChildren())
		{
			if(!topLevelNode.contents.empty())
				action(topLevelNode.contents);
			return;
		}
		std::vector<OctTreeIndex> openList;
		std::vector<OctTreeIndex> results;
		openList.reserve(m_nodes.size() / 8);
		results.reserve(m_nodes.size() / 8);
		openList.push_back(topLevelIndex);
		while(!openList.empty())
		{
			const OctTreeIndex index = openList.back();
			openList.pop_back();
			const OctTreeNode& node = m_nodes[index];
			const CuboidArray<8>& cuboids = node.cuboids;
			Eigen::Array<bool, 1, 8> contained = cuboids.indicesOfContainedCuboids(queryShape);
			Eigen::Array<bool, 1, 8> intersecting = cuboids.indicesOfIntersectingCuboids(queryShape);
			if(visionCuboids != nullptr)
			{
				Eigen::Array<bool, 1, 8> notIntersectingWithVisionCuboidsBoundry = !cuboids.indicesOfIntersectingCuboids(visionCuboidsBoundry);
				contained -= notIntersectingWithVisionCuboidsBoundry;
				intersecting -= notIntersectingWithVisionCuboidsBoundry;
			}
			for(uint i = 0; i < 8; i++)
			{
				const OctTreeIndex& child = node.children[i];
				if(contained[i])
					results.emplace_back(child);
				else if(intersecting[i])
				{
					if(m_nodes[child].hasChildren())
						openList.push_back(child);
					else
						results.emplace_back(child);
				}
			}
		}
		if(visionCuboids != nullptr)
		{
			// Filter vision cuboids in a seperate loop for locality of both data and instructions.
			// TODO: profile filtering in main loop for culling at branches rather then just at leaves.
			std::vector<OctTreeIndex> filteredResults;
			filteredResults.reserve(results.size());
			for(const OctTreeIndex& index : results)
			{
				Cuboid cuboid = m_nodes[index].cuboid;
				if(visionCuboids->intersects(cuboid))
					filteredResults.push_back(index);
			}
			results = std::move(filteredResults);
		}
		// Pass locationBucket to action after prefetching the next bucket.
		for(const OctTreeIndex& index : results)
		{
			OctTreeIndex next = index + 1;
			if(next != results.size() - 1)
				m_nodes[next].contents.prefetch();
			action(m_nodes[index].contents);
		}
	}
};