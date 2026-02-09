#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/cuboidSet.h"
#include "../strongInteger.h"
#include "../json.h"
#include "../config/config.h"
#include "strongVector.h"
#include "smallMap.h"
#include "smallSet.h"
#include "bitset.h"
#include "strongArray.h"

class RTreeBoolean
{
	static constexpr int nodeSize = Config::rtreeNodeSize;
	using BitSet = BitSet64;
	class Node
	{
		CuboidArray<nodeSize> m_cuboids;
		StrongArray<RTreeNodeIndex, RTreeArrayIndex, nodeSize> m_childIndices;
		RTreeNodeIndex m_parent;
		RTreeArrayIndex m_leafEnd = RTreeArrayIndex::create(0);
		RTreeArrayIndex m_childBegin = RTreeArrayIndex::create(nodeSize);
		// TODO: store sort order?
	public:
		[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
		[[nodiscard]] const auto& getChildIndices() const { return m_childIndices; }
		[[nodiscard]] const auto& getParent() const { return m_parent; }
		[[nodiscard]] int getLeafCount() const { return m_leafEnd.get(); }
		[[nodiscard]] int getChildCount() const { return nodeSize - m_childBegin.get(); }
		[[nodiscard]] int unusedCapacity() const { return (m_childBegin - m_leafEnd).get(); }
		[[nodiscard]] int sortOrder() const { return m_cuboids.boundry().getCenter().hilbertNumber(); };
		[[nodiscard]] RTreeArrayIndex offsetFor(const RTreeNodeIndex& index) const;
		[[nodiscard]] RTreeArrayIndex offsetOfFirstChild() const { return m_childBegin; }
		[[nodiscard]] bool empty() const { return unusedCapacity() == nodeSize; }
		[[nodiscard]] int getLeafVolume() const;
		[[nodiscard]] int getNodeVolume() const;
		[[nodiscard]] CuboidSet getLeafCuboids() const;
		[[nodiscard]] bool hasChildren() const;
		void updateChildIndex(const RTreeNodeIndex& oldIndex, const RTreeNodeIndex& newIndex);
		void insertLeaf(const Cuboid& cuboid);
		void insertBranch(const Cuboid& cuboid, const RTreeNodeIndex& index);
		void eraseBranch(const RTreeArrayIndex& offset);
		void eraseLeaf(const RTreeArrayIndex& offset);
		void eraseByMask(BitSet& mask);
		void eraseLeavesByMask(BitSet mask);
		void clear();
		void setParent(const RTreeNodeIndex& index) { m_parent = index; }
		void updateLeaf(const RTreeArrayIndex& offset, const Cuboid& cuboid);
		void updateBranchBoundry(const RTreeArrayIndex& offset, const Cuboid& cuboid);
		[[nodiscard]] __attribute__((noinline)) std::string toString();
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Node, m_cuboids, m_childIndices, m_parent, m_leafEnd, m_childBegin);
	};
	StrongVector<Node, RTreeNodeIndex> m_nodes;
	SmallSet<RTreeNodeIndex> m_emptySlots;
	SmallSet<RTreeNodeIndex> m_toComb;
	[[nodiscard]] std::tuple<Cuboid, RTreeArrayIndex, RTreeArrayIndex> findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const;
	[[nodiscard]] SmallSet<Cuboid> gatherLeavesRecursive(const RTreeNodeIndex& parent) const;
	void destroyWithChildren(const RTreeNodeIndex& index);
	void tryToMergeLeaves(Node& parent);
	void clearAllContained(const RTreeNodeIndex& index, const Cuboid& cuboid);
	void addToNodeRecursive(const RTreeNodeIndex& index, const Cuboid& cuboid);
	// Removes intercepting leaves and contained branches. Intercepting branches are added to openList.
	void removeFromNode(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList);
	void merge(const RTreeNodeIndex& destination, const RTreeNodeIndex& source);
	// Iterate m_toComb and try to recursively merge leaves.
	// Then check for single child nodes and splice them out. If the child is a leaf re-add the parent to m_toComb.
	// Finally check for child branches that can be merged upwards. If any are merged re-add parent to m_toComb.
	void comb();
	// Copy over empty slots, while updating stored childIndices in parents of nodes moved. Then truncate m_nodes.
	void defragment();
	// Sort m_nodes by hilbert order of center. The node in position 0 is the top level and never moves.
	void sort();
	static void addIntersectedChildrenToOpenList(const Node& node, BitSet& intersecting, SmallSet<RTreeNodeIndex>& openList);
public:
	RTreeBoolean() { m_nodes.add(); m_nodes.back().setParent(RTreeNodeIndex::null()); }
	void beforeJsonLoad();
	void insert(const auto& shape) { assert(!query(shape)); maybeInsert(shape); }
	void remove(const auto& shape) { assert(query(shape)); maybeRemove(shape); }
	void maybeInsert(const Cuboid& cuboid);
	void maybeRemove(const Cuboid& cuboid);
	// TODO: rewrite as batch descent.
	void maybeRemove(const CuboidSet& cuboids) { for(const Cuboid& cuboid : cuboids) maybeRemove(cuboid); }
	void maybeInsert(const CuboidSet& cuboids) { for(const Cuboid& cuboid : cuboids) maybeInsert(cuboid); }
	void maybeInsert(Cuboid&& cuboid) { const Cuboid copy = cuboid; maybeInsert(copy); }
	void maybeRemove(Cuboid&& cuboid) { const Cuboid copy = cuboid; maybeRemove(copy); }
	void maybeInsert(const Point3D& point) { const Cuboid cuboid = Cuboid(point, point); maybeInsert(cuboid); }
	void maybeRemove(const Point3D& point) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid); }
	void clear();
	void prepare();
	void queryForEach(const auto& shape, auto&& action) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			auto intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			BitSet bitset = BitSet::create(intersectMask);
			const auto leafCount = node.getLeafCount();
			BitSet leafBitSet = bitset;
			leafBitSet.clearAllAfterInclusive(leafCount);
			while(!leafBitSet.empty())
			{
				action(nodeCuboids[leafBitSet.getNextAndClear()]);
			}
			bitset.clearAllBefore(leafCount);
			if(!bitset.empty())
				addIntersectedChildrenToOpenList(node, bitset, openList);
		}
	}
	[[nodiscard]] bool canPrepare() const;
	[[nodiscard]] bool empty() const  { return nodeCount() == 1 && leafCount() == 0; }
	[[nodiscard]] CuboidSet toCuboidSet() const;
	[[nodiscard]] bool query(const Point3D& begin, const Point3D& end) const;
	template<typename ShapeT>
	[[nodiscard]] bool query(const ShapeT& shape) const;
	template<typename ShapeT>
	[[nodiscard]] Cuboid queryGetLeaf(const ShapeT& shape) const;
	template<typename ShapeT>
	[[nodiscard]] Point3D queryGetPoint(const ShapeT& shape) const;
	// Shapes should be an iterable collection supported by CuboidArray's intersection check.
	// Returns a bitset with 1 set for each shape which intersects something.
	template<typename ShapeT>
	[[nodiscard]] std::vector<bool> batchQuery(const ShapeT& shapes) const;
	template<typename ShapeT>
	[[nodiscard]] CuboidSet queryGetLeaves(const ShapeT& shape) const;
	template<typename ShapeT>
	[[nodiscard]] CuboidSet queryGetIntersection(const ShapeT& shape) const;
	template<typename ShapeT>
	[[nodiscard]] Cuboid queryGetLeafWithCondition(const ShapeT& shape, auto&& condition) const;
	template<typename ShapeT>
	[[nodiscard]] Point3D queryGetPointWithCondition(const ShapeT& shape, auto&& condition) const;
	void queryRemove(CuboidSet& set) const;
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) int nodeCount() const { return m_nodes.size() - m_emptySlots.size(); }
	[[nodiscard]] __attribute__((noinline)) int leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(int i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(int i, int o) const;
	[[nodiscard]] __attribute__((noinline)) const RTreeNodeIndex& getNodeChild(int i, int o) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) int totalLeafVolume() const;
	[[nodiscard]] __attribute__((noinline)) int totalNodeVolume() const;
	__attribute__((noinline)) void assertAllLeafsAreUnique() const;
	[[nodiscard]] static __attribute__((noinline)) int getNodeSize();
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RTreeBoolean, m_nodes, m_emptySlots, m_toComb);
};