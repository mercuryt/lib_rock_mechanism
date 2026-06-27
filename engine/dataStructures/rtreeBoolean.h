#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/cuboidSet.h"
#include "../strongInteger.h"
#include "../json.h"
#include "../config/config.h"
#include "../allocators/watermarkingStack.h"
#include "strongVector.h"
#include "smallMap.h"
#include "smallSet.h"
#include "bitset.h"
#include "strongArray.h"

class RTreeBoolean
{
	static constexpr int nodeSize = Config::rtreeNodeSize;
	using BitSet = BitSet64;
	using OpenList = ThreadStripedWatermarkingStack<RTreeNodeIndex>;
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
		[[nodiscard]] RTreeArrayIndex offsetFor(const RTreeNodeIndex index) const;
		[[nodiscard]] RTreeArrayIndex offsetOfFirstChild() const { return m_childBegin; }
		[[nodiscard]] bool empty() const { return unusedCapacity() == nodeSize; }
		[[nodiscard]] int getLeafVolume() const;
		[[nodiscard]] int getNodeVolume() const;
		[[nodiscard]] CuboidSet getLeafCuboids() const;
		[[nodiscard]] bool hasChildren() const;
		void updateChildIndex(const RTreeNodeIndex oldIndex, const RTreeNodeIndex newIndex);
		void insertLeaf(const Cuboid cuboid);
		void insertBranch(const Cuboid cuboid, const RTreeNodeIndex index);
		void eraseBranch(const RTreeArrayIndex offset);
		void eraseLeaf(const RTreeArrayIndex offset);
		void eraseByMask(BitSet& mask);
		void eraseLeavesByMask(BitSet mask);
		void clear();
		void setParent(const RTreeNodeIndex index) { m_parent = index; }
		void updateLeaf(const RTreeArrayIndex offset, const Cuboid cuboid);
		void updateBranchBoundry(const RTreeArrayIndex offset, const Cuboid cuboid);
		void log() const;
		GDB_CALLABLE std::string toS() const;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Node, m_cuboids, m_childIndices, m_parent, m_leafEnd, m_childBegin);
	};
	StrongVector<Node, RTreeNodeIndex> m_nodes;
	SmallSet<RTreeNodeIndex> m_emptySlots;
	SmallSet<RTreeNodeIndex> m_toComb;
	[[nodiscard]] std::tuple<Cuboid, RTreeArrayIndex, RTreeArrayIndex> findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const;
	[[nodiscard]] SmallSet<Cuboid> gatherLeavesRecursive(const RTreeNodeIndex parent) const;
	void destroyWithChildren(const RTreeNodeIndex index);
	void tryToMergeLeaves(Node& parent);
	void clearAllContained(const RTreeNodeIndex index, const Cuboid cuboid);
	void addToNodeRecursive(const RTreeNodeIndex index, const Cuboid cuboid);
	// Removes intercepting leaves and contained branches. Intercepting branches are added to openList.
	void removeFromNode(const RTreeNodeIndex index, const Cuboid cuboid, OpenList& openList);
	void merge(const RTreeNodeIndex destination, const RTreeNodeIndex source);
	// Iterate m_toComb and try to recursively merge leaves.
	// Then check for single child nodes and splice them out. If the child is a leaf re-add the parent to m_toComb.
	// Finally check for child branches that can be merged upwards. If any are merged re-add parent to m_toComb.
	void comb();
	// Copy over empty slots, while updating stored childIndices in parents of nodes moved. Then truncate m_nodes.
	void defragment();
	// Sort m_nodes by hilbert order of center. The node in position 0 is the top level and never moves.
	void sort();
	static void addIntersectedChildrenToOpenList(const Node& node, BitSet& intersecting, OpenList& openList);
	// Customization point.
	[[nodiscard]] bool canMerge(const Cuboid, const Cuboid) const { return true; }
public:
	RTreeBoolean() { m_nodes.add(); m_nodes.back().setParent(RTreeNodeIndex::null()); }
	void beforeJsonLoad();
	void insert(const auto& shape) { assert(!query(shape)); maybeInsert(shape); }
	void remove(const auto& shape) { assert(query(shape)); maybeRemove(shape); }
	void maybeInsert(const Cuboid cuboid);
	void maybeRemove(const Cuboid cuboid);
	// TODO: rewrite as batch descent.
	void maybeRemove(const CuboidSet& cuboids) { for(const Cuboid cuboid : cuboids) maybeRemove(cuboid); }
	void maybeInsert(const CuboidSet& cuboids) { for(const Cuboid cuboid : cuboids) maybeInsert(cuboid); }
	void maybeInsert(const Point3D point) { const Cuboid cuboid = Cuboid(point, point); maybeInsert(cuboid); }
	void maybeRemove(const Point3D point) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid); }
	void clear();
	void prepare();
	void queryForEach(const auto& shape, auto&& action) const
	{
		OpenList openList;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.popAndReturnBack();
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
	void forEach(auto&& action) const
	{
		const int nodeEnd = m_nodes.size();
		for(RTreeNodeIndex nodeIndex{0}; nodeIndex != nodeEnd; ++nodeIndex)
			// If empty slots were sorted this could be done in one pass.
			if(!m_emptySlots.contains(nodeIndex))
			{
				const Node& node = m_nodes[nodeIndex];
				const auto& cuboids = node.getCuboids();
				const int leafEnd = node.getLeafCount();
				for(RTreeArrayIndex arrayIndex{0}; arrayIndex != leafEnd; ++arrayIndex)
					action(cuboids[arrayIndex.get()]);
			}
	}
	[[nodiscard]] bool canPrepare() const;
	[[nodiscard]] bool empty() const { return nodeCount() == 1 && leafCount() == 0; }
	[[nodiscard]] CuboidSet toCuboidSet() const;
private:
	template<typename ShapeT>
	[[nodiscard]] bool queryBody(const ShapeT shape) const;
public:
	[[nodiscard]] bool query(const Point3D begin, const Point3D end) const;
	[[nodiscard]] bool query(const Point3D shape) const;
	[[nodiscard]] bool query(const Cuboid shape) const;
	[[nodiscard]] bool query(const CuboidSet& shape) const;
	[[nodiscard]] bool query(const ParamaterizedLine& shape) const;
private:
	template<typename ShapeT>
	[[nodiscard]] Cuboid queryGetLeafBody(ShapeT shape) const;
public:
	Cuboid queryGetLeaf(Point3D point) const;
	Cuboid queryGetLeaf(Cuboid cuboid) const;
	Cuboid queryGetLeaf(const CuboidSet& cuboids) const;
	template<typename ShapeT>
	[[nodiscard]] Point3D queryGetPoint(ShapeT&& shape) const;
	// Shapes should be an iterable collection supported by CuboidArray's intersection check.
	// Returns a bitset with 1 set for each shape which intersects something.
	template<typename ShapeT>
	[[nodiscard]] std::vector<bool> batchQuery(const ShapeT& shapes) const;
private:
	template<typename ShapeT>
	[[nodiscard]] CuboidSet queryGetLeavesBody(ShapeT shape) const;
public:
	[[nodiscard]] CuboidSet queryGetLeaves(const CuboidSet& cuboids) const;
	[[nodiscard]] CuboidSet queryGetLeaves(const Cuboid cuboid) const;
	[[nodiscard]] CuboidSet queryGetLeaves(const Sphere sphere) const;
private:
	template<typename ShapeT>
	[[nodiscard]] CuboidSet queryGetIntersectionBody(ShapeT shape) const;
public:
	[[nodiscard]] CuboidSet queryGetIntersection(const CuboidSet& cuboids) const;
	[[nodiscard]] CuboidSet queryGetIntersection(const Cuboid cuboid) const;
	template<typename ShapeT>
	[[nodiscard]] Cuboid queryGetLeafWithCondition(ShapeT&& shape, auto&& condition) const;
	template<typename ShapeT>
	[[nodiscard]] Point3D queryGetPointWithCondition(ShapeT&& shape, auto&& condition) const;
	void queryRemove(CuboidSet& set) const;
	template<typename ActionT>
	void forEachCuboid(ActionT&& action) const;
	// For test and debug.
	GDB_CALLABLE int nodeCount() const { return m_nodes.size() - m_emptySlots.size(); }
	GDB_CALLABLE int leafCount() const;
	GDB_CALLABLE const Node& getNode(int i) const;
	GDB_CALLABLE std::string toS() const;
	GDB_CALLABLE std::string operator()() const;
	GDB_CALLABLE const Cuboid getNodeCuboid(int i, int o) const;
	GDB_CALLABLE const RTreeNodeIndex getNodeChild(int i, int o) const;
	GDB_CALLABLE bool queryPoint(int x, int y, int z) const;
	GDB_CALLABLE int totalLeafVolume() const;
	GDB_CALLABLE int totalNodeVolume() const;
	GDB_CALLABLE void assertAllLeafsAreUnique() const;
	[[nodiscard]] static GDB_CALLABLE int getNodeSize();
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RTreeBoolean, m_nodes, m_emptySlots, m_toComb);
};