#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../strongInteger.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/smallMap.h"
#include "../json.h"
struct RTreeDataConfig
{
	bool splitAndMerge = true;
	bool leavesCanOverlap = false;
};

template <typename T>
concept Sortable = requires(T a, T b) {
    { a < b } -> std::same_as<bool>; // Check if operator< exists and returns bool
};

template<Sortable T, RTreeDataConfig config = RTreeDataConfig()>
class RTreeData
{
	static constexpr uint nodeSize = 4;
	using IndexWidth = uint16_t;
	class Index final : public StrongInteger<Index, IndexWidth>
	{
	public:
		struct Hash { [[nodiscard]] size_t operator()(const Index& index) const { return index.get(); } };
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Index, data);
	};
	using ArrayIndexWidth = uint8_t;
	class ArrayIndex final : public StrongInteger<ArrayIndex, ArrayIndexWidth>
	{
	public:
		struct Hash { [[nodiscard]] size_t operator()(const ArrayIndex& index) const { return index.get(); } };
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ArrayIndex, data);
	};
	union DataOrChild { T::Primitive data; Index::Primitive child; };
	class Node
	{
		CuboidArray<nodeSize> m_cuboids;
		std::array<DataOrChild, nodeSize> m_dataAndChildIndices;
		Index m_parent;
		ArrayIndex m_leafEnd = ArrayIndex::create(0);
		ArrayIndex m_childBegin = ArrayIndex::create(nodeSize);
		// TODO: store sort order?
	public:
		[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
		[[nodiscard]] const auto& getDataAndChildIndices() const { return m_dataAndChildIndices; }
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
		[[nodiscard]] Json toJson() const;
		void load(const Json& data);
		void updateChildIndex(const Index& oldIndex, const Index& newIndex);
		void insertLeaf(const Cuboid& cuboid, const T& value);
		void insertBranch(const Cuboid& cuboid, const Index& index);
		void eraseBranch(const ArrayIndex& offset);
		void eraseLeaf(const ArrayIndex& offset);
		void eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void eraseLeavesByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void clear();
		void setParent(const Index& index) { m_parent = index; }
		void updateLeaf(const ArrayIndex& offset, const Cuboid& cuboid);
		void updateValue(const ArrayIndex& offset, const T& value);
		void updateLeafWithValue(const ArrayIndex& offset, const Cuboid& cuboid, const T& value);
		void updateBranchBoundry(const ArrayIndex& offset, const Cuboid& cuboid);
		[[nodiscard]] __attribute__((noinline)) std::string toString();
	};
	StrongVector<Node, Index> m_nodes;
	SmallSet<Index> m_emptySlots;
	SmallSet<Index> m_toComb;
	T m_nullValue;
	[[nodiscard]] std::tuple<Cuboid, ArrayIndex, ArrayIndex> findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const;
	[[nodiscard]] SmallMap<Cuboid, T> gatherLeavesRecursive(const Index& parent) const;
	void destroyWithChildren(const Index& index);
	void tryToMergeLeaves(Node& parent);
	void clearAllContained(Node& parent, const Cuboid& cuboid);
	void clearAllContainedWithValueRecursive(Node& parent, const Cuboid& cuboid, const T& value);
	void addToNodeRecursive(const Index& index, const Cuboid& cuboid, const T& value);
	// Removes intercepting leaves and contained branches. Intercepting branches are added to openList.
	void removeFromNode(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList);
	void removeFromNodeWithValue(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList, const T& value);
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
	RTreeData() { m_nodes.add(); m_nodes.back().setParent(Index::null()); }
	void maybeInsert(const Cuboid& cuboid, const T& value);
	void maybeRemove(const Cuboid& cuboid, const T& value);
	void maybeRemove(const Cuboid& cuboid);
	void maybeInsert(Cuboid&& cuboid, const T& value) { const Cuboid copy = cuboid; maybeInsert(copy, value); }
	void maybeRemove(Cuboid&& cuboid, const T& value) { const Cuboid copy = cuboid; maybeRemove(copy, value); }
	void maybeRemove(Cuboid&& cuboid) { const Cuboid copy = cuboid; maybeRemove(copy); }
	void maybeInsert(const Point3D& point, const T& value) { const Cuboid cuboid = Cuboid(point, point); maybeInsert(cuboid, value); }
	void maybeRemove(const Point3D& point, const T& value) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid, value); }
	void maybeRemove(const Point3D& point) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid); }
	void maybeInsertOrOverwrite(const Point3D& point, const T& value) { maybeRemove(point); maybeInsert(point, value); }
	void prepare();
	[[nodiscard]] Json toJson() const;
	void load(const Json& data);
	void update(const auto& shape, const T& oldValue, const T& newValue)
	{

		SmallSet<Index> openList;
		openList.insert(Index::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			const auto& begin = interceptMask.begin();
			const auto leafEnd = begin + leafCount;
			for(auto iter = begin; iter != leafEnd; ++iter)
				if(*iter)
				{
					uint8_t offset = iter - begin;
					if(nodeDataAndChildIndices[offset].data == oldValue.get())
						node.updateValue(ArrayIndex::create(offset), newValue);
				}
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			const auto end = interceptMask.end();
			for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
				if(*iter)
				{
					uint8_t offset = iter - begin;
					openList.insert(Index::create(nodeDataAndChildIndices[offset].child));
				}
		}
	}
	[[nodiscard]] bool queryAny(const auto& shape) const
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
			const auto childCount = node.getChildCount();
			// TODO: Would it be better to check the whole intercept mask and continue if all are empty before checking leafs?
			if(childCount == 0 || !interceptMask.tail(childCount).any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			auto begin = interceptMask.begin();
			auto end = begin + nodeSize;
			if(offsetOfFirstChild == 0)
			{
				// Unrollable version for nodes where every slot is a child.
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
			else
			{
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
		}
		return false;
	}
	[[nodiscard]]const T queryGetOne(const auto& shape) const
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
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						return T::create(nodeDataAndChildIndices[iter - begin].data);
			}
			auto begin = interceptMask.begin();
			const auto end = begin + nodeSize;
			if(offsetOfFirstChild == 0)
			{
				// Unrollable version for nodes where every slot is a child.
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
			else
			{
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
		}
		return m_nullValue;
	}
	[[nodiscard]] const SmallSet<T> queryGetAll(const auto& shape) const
	{
		SmallSet<Index> openList;
		SmallSet<T> output;
		openList.insert(Index::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						output.insertNonunique(T::create(nodeDataAndChildIndices[iter - begin].data));
			}
			const auto begin = interceptMask.begin();
			const auto end = begin + nodeSize;
			if(offsetOfFirstChild == 0)
			{
				// Unrollable version for nodes where every slot is a child.
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
			else
			{
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
		}
		output.makeUnique();
		return output;
	}
	[[nodiscard]] const SmallSet<std::pair<Cuboid, T>> queryGetAllWithCuboids(const auto& shape) const
	{
		SmallSet<Index> openList;
		SmallSet<std::pair<Cuboid, T>> output;
		openList.insert(Index::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						output.emplace(nodeCuboids[offset], T::create(nodeDataAndChildIndices[offset].data));
					}
			}
			const auto begin = interceptMask.begin();
			const auto end = begin + nodeSize;
			if(offsetOfFirstChild == 0)
			{
				// Unrollable version for nodes where every slot is a child.
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
			else
			{
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openList.insert(Index::create(nodeDataAndChildIndices[iter - begin].child));
			}
		}
		return output;
	}
	[[nodiscard]] std::vector<bool> batchQueryAny(const auto& shapes) const
	{
		std::vector<bool> output;
		output.resize(shapes.size());
		SmallMap<Index, SmallSet<uint>> openList;
		openList.insert(Index::create(0), {});
		auto& rootNodeCandidateList = openList.back().second;
		rootNodeCandidateList.resize(shapes.size());
		std::ranges::iota(rootNodeCandidateList.getVector(), 0);
		while(!openList.empty())
		{
			auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& nodeChildren = node.getDataAndChildIndices();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			for(const uint& shapeIndex : candidates)
			{
				if(output[shapeIndex])
					// This shape has already intersected with a leaf, no need to check further.
					continue;
				const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
				// Record a leaf intersection.
				const auto leafCount = node.getLeafCount();
				if(leafCount != 0 && interceptMask.head(leafCount).any())
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
							openList.getOrCreate(Index::create(nodeChildren[iter - begin].child)).insert(shapeIndex);
				}
				else
				{
					for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(Index::create(nodeChildren[iter - begin].child)).insert(shapeIndex);
				}
			}
			// Pop back at the end so we can use candidates as a reference and not have to make a move-copy.
			openList.popBack();
		}
		return output;
	}
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) uint nodeCount() const { return m_nodes.size() - m_emptySlots.size(); }
	[[nodiscard]] __attribute__((noinline)) uint leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(uint i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) const Index& getNodeChild(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(uint x, uint y, uint z, const T& value) const;
	[[nodiscard]] __attribute__((noinline)) uint totalLeafVolume() const;
	[[nodiscard]] __attribute__((noinline)) uint totalNodeVolume() const;
	__attribute__((noinline)) void assertAllLeafsAreUnique() const;
	[[nodiscard]] static __attribute__((noinline)) uint getNodeSize();
};
template<typename T, RTreeDataConfig config = RTreeDataConfig()>
void to_json(Json& data, const RTreeData<T, config>& tree) { data = tree.toJson(); }
template<typename T, RTreeDataConfig config = RTreeDataConfig()>
void from_json(const Json& data, RTreeData<T, config>& tree) { tree.load(data); }
