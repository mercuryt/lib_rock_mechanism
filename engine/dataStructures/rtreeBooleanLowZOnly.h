/*
	The only differences between this class and RTreeBoolean are in tryToMergeLeaves and the two query methods.
*/
#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/paramaterizedLine.h"
#include "../strongInteger.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/smallMap.hpp"
#include "../dataStructures/smallSet.hpp"
#include "../config/config.h"

class RTreeBooleanLowZOnly
{
	static constexpr int nodeSize = Config::rtreeNodeSize;
	using IndexWidth = int;
	class Index : public StrongInteger<Index, IndexWidth, INT_MAX, 0>
	{
	public:
		struct Hash { [[nodiscard]] size_t operator()(const Index& index) const { return index.get(); } };
	};
	using ArrayIndexWidth = int;
	class ArrayIndex : public StrongInteger<ArrayIndex, ArrayIndexWidth, nodeSize + 1, 0>
	{
	public:
		struct Hash { [[nodiscard]] size_t operator()(const ArrayIndex& index) const { return index.get(); } };
	};
	class Node
	{
		CuboidArray<nodeSize> m_cuboids;
		std::array<Index, nodeSize> m_childIndices;
		Index m_parent;
		ArrayIndex m_leafEnd = ArrayIndex::create(0);
		ArrayIndex m_childBegin = ArrayIndex::create(nodeSize);
		// TODO: store sort order?
	public:
		[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
		[[nodiscard]] const auto& getChildIndices() const { return m_childIndices; }
		[[nodiscard]] const auto& getParent() const { return m_parent; }
		[[nodiscard]] int getLeafCount() const { return m_leafEnd.get(); }
		[[nodiscard]] int getChildCount() const { return nodeSize - m_childBegin.get(); }
		[[nodiscard]] int unusedCapacity() const { return (m_childBegin - m_leafEnd).get(); }
		[[nodiscard]] int sortOrder() const { return m_cuboids.boundry().getCenter().hilbertNumber(); };
		[[nodiscard]] ArrayIndex offsetFor(const Index& index) const;
		[[nodiscard]] ArrayIndex offsetOfFirstChild() const { return m_childBegin; }
		[[nodiscard]] bool empty() const { return unusedCapacity() == nodeSize; }
		[[nodiscard]] int getLeafVolume() const;
		[[nodiscard]] int getNodeVolume() const;
		void updateChildIndex(const Index& oldIndex, const Index& newIndex);
		void insertLeaf(const Cuboid& cuboid);
		void insertBranch(const Cuboid& cuboid, const Index& index);
		void eraseBranch(const ArrayIndex& offset);
		void eraseLeaf(const ArrayIndex& offset);
		void eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void eraseLeavesByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void clear();
		void setParent(const Index& index) { m_parent = index; }
		void updateLeaf(const ArrayIndex& offset, const Cuboid& cuboid);
		void updateBranchBoundry(const ArrayIndex& offset, const Cuboid& cuboid);
		[[nodiscard]] __attribute__((noinline)) std::string toString();
	};
	StrongVector<Node, Index> m_nodes;
	SmallSet<Index> m_emptySlots;
	SmallSet<Index> m_toComb;
	[[nodiscard]] std::tuple<Cuboid, ArrayIndex, ArrayIndex> findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const;
	[[nodiscard]] SmallSet<Cuboid> gatherLeavesRecursive(const Index& parent) const;
	void destroyWithChildren(const Index& index);
	void tryToMergeLeaves(Node& parent);
	void clearAllContained(const Index& index, const Cuboid& cuboid);
	void addToNodeRecursive(const Index& index, const Cuboid& cuboid);
	// Removes intercepting leaves and contained branches. Intercepting branches are added to openList.
	void removeFromNode(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList);
	void merge(const Index& destination, const Index& source);
	// Iterate m_toComb and try to recursively merge leaves.
	// Then check for single child nodes and splice them out. If the child is a leaf re-add the parent to m_toComb.
	// Finally check for child branches that can be merged upwards. If any are merged re-add parent to m_toComb.
	void comb();
	// Copy over empty slots, while updating stored childIndices in parents of nodes moved. Then truncate m_nodes.
	void defragment();
	// Sort m_nodes by hilbert order of center. The node in position 0 is the top level and never moves.
	void sort();
public:
	RTreeBooleanLowZOnly() { m_nodes.add(); m_nodes.back().setParent(Index::null()); }
	void maybeInsert(const Cuboid& cuboid);
	void maybeRemove(const Cuboid& cuboid);
	void maybeInsert(Cuboid&& cuboid) { const Cuboid copy = cuboid; maybeInsert(copy); }
	void maybeRemove(Cuboid&& cuboid) { const Cuboid copy = cuboid; maybeRemove(copy); }
	void maybeInsert(const Point3D& point) { const Cuboid cuboid = Cuboid(point, point); maybeInsert(cuboid); }
	void maybeRemove(const Point3D& point) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid); }
	void prepare();
	[[nodiscard]] bool query(const Point3D& begin, const Point3D& end) const { return query(ParamaterizedLine(begin, end)); }
	[[nodiscard]] bool query(const auto& shape) const
	{
		SmallSet<Index> openList;
		openList.insert(Index::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			// This line is the only place where this method differs from RTreeBoolen.
			if(leafCount != 0 && interceptMask.head(leafCount).any() && nodeCuboids.indicesOfIntersectingCuboidsLowZOnly(shape).head(leafCount).any())
				return true;
			// TODO: Would it be better to check the whole intercept mask and continue if all are empty before checking leafs?
			const auto childCount = node.getChildCount();
			if(childCount == 0 || !interceptMask.tail(childCount).any())
				continue;
			const auto& nodeChildren = node.getChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			auto begin = interceptMask.begin();
			auto end = begin + nodeSize;
			if(offsetOfFirstChild == 0)
			{
				// Unrollable version for nodes where every slot is a child.
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openList.insert(nodeChildren[iter - begin]);
			}
			else
			{
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openList.insert(nodeChildren[iter - begin]);
			}
		}
		return false;
	}
	// Shapes should be an iterable collection supported by CuboidArray's intersection check.
	// Returns a bitset with 1 set for each shape which intersects something.
	[[nodiscard]] std::vector<bool> batchQuery(const auto& shapes) const
	{
		std::vector<bool> output;
		output.resize(shapes.size());
		SmallMap<Index, SmallSet<int>> openList;
		openList.insert(Index::create(0), {});
		auto& rootNodeCandidateList = openList.back().second;
		rootNodeCandidateList.resize(shapes.size());
		std::ranges::iota(rootNodeCandidateList.getVector(), 0);
		while(!openList.empty())
		{
			auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& nodeChildren = node.getChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			for(const int& shapeIndex : candidates)
			{
				if(output[shapeIndex])
					// This shape has already intersected with a leaf, no need to check further.
					continue;
				const auto& shape = shapes[shapeIndex];
				const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
				// Record a leaf intersection.
				const auto leafCount = node.getLeafCount();
				// This line is the only place where this method differs from RTreeBoolen.
				if(leafCount != 0 && interceptMask.head(leafCount).any() && nodeCuboids.indicesOfIntersectingCuboidsLowZOnly(shape).head(leafCount).any())
				{
					output[shapeIndex] = true;
					continue;
				}
				// Record all node intersections.
				const auto childCount = node.getChildCount();
				if(childCount == 0 || !interceptMask.tail(childCount).any())
					continue;
				auto begin = interceptMask.begin();
				const auto end = begin + nodeSize;
				if(offsetOfFirstChild == 0)
				{
					// Unrollable version for nodes where every slot is a child.
					for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(nodeChildren[iter - begin]).insert(shapeIndex);
				}
				else
				{
					for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(nodeChildren[iter - begin]).insert(shapeIndex);
				}
			}
			// Pop back at the end so we can use candidates as a reference and not have to make a move-copy.
			openList.popBack();
		}
		return output;
	}
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) int nodeCount() const { return m_nodes.size() - m_emptySlots.size(); }
	[[nodiscard]] __attribute__((noinline)) int leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(int i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(int i, int o) const;
	[[nodiscard]] __attribute__((noinline)) const Index& getNodeChild(int i, int o) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) int totalLeafVolume() const;
	[[nodiscard]] __attribute__((noinline)) int totalNodeVolume() const;
	__attribute__((noinline)) void assertAllLeafsAreUnique() const;
	[[nodiscard]] static __attribute__((noinline)) int getNodeSize();
};