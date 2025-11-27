#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/cuboidSet.h"
#include "../strongInteger.h"
#include "../json.h"
#include "strongVector.h"
#include "smallMap.h"
#include "smallSet.h"
#include "bitset.h"

class RTreeBoolean
{
	static constexpr uint nodeSize = 64;
	using IndexWidth = uint16_t;
	class Index : public StrongInteger<Index, IndexWidth>
	{
	public:
		struct Hash { [[nodiscard]] size_t operator()(const Index& index) const { return index.get(); } };
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Index, data);
	};
	using ArrayIndexWidth = uint8_t;
	class ArrayIndex : public StrongInteger<ArrayIndex, ArrayIndexWidth>
	{
	public:
		struct Hash { [[nodiscard]] size_t operator()(const ArrayIndex& index) const { return index.get(); } };
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ArrayIndex, data);
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
		[[nodiscard]] uint getLeafVolume() const;
		[[nodiscard]] uint getNodeVolume() const;
		[[nodiscard]] CuboidSet getLeafCuboids() const;
		void updateChildIndex(const Index& oldIndex, const Index& newIndex);
		void insertLeaf(const Cuboid& cuboid);
		void insertBranch(const Cuboid& cuboid, const Index& index);
		void eraseBranch(const ArrayIndex& offset);
		void eraseLeaf(const ArrayIndex& offset);
		void eraseByMask(BitSet64& mask);
		void eraseLeavesByMask(BitSet64 mask);
		void clear();
		void setParent(const Index& index) { m_parent = index; }
		void updateLeaf(const ArrayIndex& offset, const Cuboid& cuboid);
		void updateBranchBoundry(const ArrayIndex& offset, const Cuboid& cuboid);
		[[nodiscard]] __attribute__((noinline)) std::string toString();
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Node, m_cuboids, m_childIndices, m_parent, m_leafEnd, m_childBegin);
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
	static void addIntersectedChildrenToOpenList(const Node& node, BitSet<uint64_t, 64>& intersecting, SmallSet<Index>& openList);
public:
	RTreeBoolean() { m_nodes.add(); m_nodes.back().setParent(Index::null()); }
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
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) uint nodeCount() const { return m_nodes.size() - m_emptySlots.size(); }
	[[nodiscard]] __attribute__((noinline)) uint leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(uint i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) const Index& getNodeChild(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) uint totalLeafVolume() const;
	[[nodiscard]] __attribute__((noinline)) uint totalNodeVolume() const;
	__attribute__((noinline)) void assertAllLeafsAreUnique() const;
	[[nodiscard]] static __attribute__((noinline)) uint getNodeSize();
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RTreeBoolean, m_nodes, m_emptySlots, m_toComb);
};