#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../strongInteger.h"
#include "../dataStructures/strongVector.h"

class RTreeBoolean
{
	static constexpr uint nodeSize = 32;
	using IndexWidth = uint16_t;
	class Index : public StrongInteger<Index, IndexWidth>
	{
	public:
		Index() = default;
		struct Hash { [[nodiscard]] size_t operator()(const Index& index) const { return index.get(); } };
	};
	using ArrayIndexWidth = uint8_t;
	class ArrayIndex : public StrongInteger<ArrayIndex, ArrayIndexWidth>
	{
	public:
		ArrayIndex() = default;
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
		void updateChildIndex(const Index& oldIndex, const Index& newIndex);
		void insertLeaf(const Cuboid& cuboid);
		void insertBranch(const Cuboid& cuboid, const Index& index);
		void eraseBranch(const ArrayIndex& offset);
		void eraseLeaf(const ArrayIndex& offset);
		void eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void clear();
		void setParent(const Index& index) { m_parent = index; }
		void updateLeaf(const ArrayIndex offset, const Cuboid& cuboid);
		void updateBranchBoundry(const ArrayIndex offset, const Cuboid& cuboid);
		[[nodiscard]] __attribute__((noinline)) std::string toString();
	};
	StrongVector<Node, Index> m_nodes;
	SmallSet<Index> m_emptySlots;
	SmallSet<Index> m_toComb;
	[[nodiscard]] std::tuple<Cuboid, ArrayIndex, ArrayIndex> findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids);
	[[nodiscard]] SmallSet<Cuboid> gatherLeavesRecursive(const Index& parent);
	void destroyWithChildren(const Index& index);
	void tryToMergeLeaves(Node& parent);
	void clearAllContained(Node& parent, const Cuboid& cuboid);
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
	RTreeBoolean() { m_nodes.add(); m_nodes.back().setParent(Index::null()); }
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
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
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						openList.insert(nodeDataAndChildIndices[iter - begin].child);
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
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) uint nodeCount() const { return m_nodes.size() - m_emptySlots.size(); }
	[[nodiscard]] __attribute__((noinline)) uint leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(uint i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) const Index& getNodeChild(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(uint x, uint y, uint z) const;
	[[nodiscard]] static __attribute__((noinline)) uint getNodeSize();
};