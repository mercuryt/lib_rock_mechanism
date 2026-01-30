#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/mapWithCuboidKeys.h"
#include "../json.h"
#include "../numericTypes/index.h"
#include "../concepts.h"
#include "../config/config.h"
#include "strongVector.hpp"
#include "strongArray.h"
#include "smallMap.h"
#include "smallSet.hpp"
#include "bitset.h"
struct RTreeDataConfig
{
	bool splitAndMerge = true;
	bool leavesCanOverlap = false;
	bool validate = false;
};

namespace RTreeDataConfigs
{
	static constexpr RTreeDataConfig noMergeOrOverlap{.splitAndMerge = false, .leavesCanOverlap = false};
	static constexpr RTreeDataConfig noMerge{.splitAndMerge = false, .leavesCanOverlap = true};
	static constexpr RTreeDataConfig noOverlap{.splitAndMerge = true, .leavesCanOverlap = false};
	static constexpr RTreeDataConfig canOverlapNoMerge{.splitAndMerge = false, .leavesCanOverlap = true};
	static constexpr RTreeDataConfig canOverlapAndMerge{.splitAndMerge = true, .leavesCanOverlap = true};
};

// TODO: create concept checking if T verifies the other template requirements.

template<Sortable T, RTreeDataConfig config = RTreeDataConfig(), typename T::Primitive nullPrimitive = T::nullPrimitive()>
class RTreeData
{
	static constexpr int nodeSize = Config::rtreeNodeSize;
	using BitSet = BitSet64;
	union DataOrChild { T::Primitive data; RTreeNodeIndex::Primitive child; };
	class Node
	{
		CuboidArray<nodeSize> m_cuboids;
		StrongArray<DataOrChild, RTreeArrayIndex, nodeSize> m_dataAndChildIndices;
		RTreeNodeIndex m_parent;
		RTreeArrayIndex m_leafEnd = RTreeArrayIndex::create(0);
		RTreeArrayIndex m_childBegin = RTreeArrayIndex::create(nodeSize);
		// TODO: store sort order?
	public:
		[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
		[[nodiscard]] const auto& getDataAndChildIndices() const { return m_dataAndChildIndices; }
		[[nodiscard]] auto& getDataAndChildIndices() { return m_dataAndChildIndices; }
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
		[[nodiscard]] Json toJson() const;
		[[nodiscard]] bool containsLeaf(const Cuboid& cuboid, const T& value) const;
		[[nodiscard]] bool hasChildren() const ;
		[[nodiscard]] bool operator==(const Node& other) const = default;
		void load(const Json& data);
		void updateChildIndex(const RTreeNodeIndex& oldIndex, const RTreeNodeIndex& newIndex);
		void insertLeaf(const Cuboid& cuboid, const T& value);
		void insertLeaf(const Point3D& point, const T& value) { insertLeaf(Cuboid{point, point}, value); }
		void insertBranch(const Cuboid& cuboid, const RTreeNodeIndex& index);
		void eraseBranch(const RTreeArrayIndex& offset);
		void eraseLeaf(const RTreeArrayIndex& offset);
		void eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void eraseByMask(BitSet mask);
		void eraseLeavesByMask(BitSet mask);
		void eraseLeavesByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void clear();
		void setParent(const RTreeNodeIndex& index) { m_parent = index; }
		void updateLeaf(const RTreeArrayIndex& offset, const Cuboid& cuboid);
		void updateValue(const RTreeArrayIndex& offset, const T& value);
		void updateLeafWithValue(const RTreeArrayIndex& offset, const Cuboid& cuboid, const T& value);
		void maybeUpdateLeafWithValue(const RTreeArrayIndex& offset, const Cuboid& cuboid, const T& value);
		void updateLeafWithValue(const RTreeArrayIndex& offset, const Point3D& point, const T& value) { updateLeafWithValue(offset, Cuboid{point, point}, value); }
		void updateBranchBoundry(const RTreeArrayIndex& offset, const Cuboid& cuboid);
		[[nodiscard]] __attribute__((noinline)) std::string toString();
	};
	StrongVector<Node, RTreeNodeIndex> m_nodes;
	SmallSet<RTreeNodeIndex> m_emptySlots;
	SmallSet<RTreeNodeIndex> m_toComb;
	[[nodiscard]] std::tuple<Cuboid, RTreeArrayIndex, RTreeArrayIndex> findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const;
	[[nodiscard]] SmallSet<std::pair<Cuboid, T>> gatherLeavesRecursive(const RTreeNodeIndex& parent) const;
	void destroyWithChildren(const RTreeNodeIndex& index);
	void tryToMergeLeaves(Node& parent);
	void clearAllContained(const RTreeNodeIndex& index, const Cuboid& cuboid);
	void clearAllContainedWithValueRecursive(Node& parent, const Cuboid& cuboid, const T& value);
	void addToNodeRecursive(const RTreeNodeIndex& index, const Cuboid& cuboid, const T& value);
	// Removes intersecting leaves and contained branches. Intersecting branches are added to openList.
	void removeFromNode(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList);
	void removeFromNodeWithValue(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList, const T& value);
	void removeFromNodeByMask(Node& node, const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
	void removeFromNodeByMask(Node& node, BitSet mask);
	void updateBoundriesMaybe(const SmallSet<RTreeNodeIndex>& indices);
	void merge(const RTreeNodeIndex& destination, const RTreeNodeIndex& source);
	// Iterate m_toComb and try to recursively merge leaves.
	// Then check for single child nodes and splice them out. If the child is a leaf re-add the parent to m_toComb.
	// Finally check for child branches that can be merged upwards. If any are merged re-add parent to m_toComb.
	void comb();
	// Copy over empty slots, while updating stored childIndices in parents of nodes moved. Then truncate m_nodes.
	void defragment();
	// Sort m_nodes by hilbert order of center. The node in position 0 is the top level and never moves.
	void sort();
	static void addIntersectedChildrenToOpenList(const Node& node, const BitSet intersecting, SmallSet<RTreeNodeIndex>& openList);
	[[nodiscard]] virtual bool canOverlap(const T&, const T&) const { return true; }
public:
	RTreeData();
	void beforeJsonLoad();
	void maybeInsert(const Cuboid& cuboid, const T& value);
	void maybeRemove(const Cuboid& cuboid, const T& value);
	void maybeRemove(const Cuboid& cuboid);
	void maybeInsert(const CuboidSet& cuboids, const T& value);
	void maybeRemove(const CuboidSet& cuboids);
	void maybeInsert(const Point3D& point, const T& value) { const Cuboid cuboid = Cuboid(point, point); maybeInsert(cuboid, value); }
	void maybeRemove(const Point3D& point, const T& value) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid, value); }
	void maybeRemove(const Point3D& point) { const Cuboid cuboid = Cuboid(point, point); maybeRemove(cuboid); }
	void maybeInsertOrOverwrite(const Point3D& point, const T& value);
	void maybeInsertOrOverwrite(const Cuboid& cuboid, const T& value);
	void insert(const auto& shape, const T& value)
	{
		if constexpr(config.leavesCanOverlap)
			assert(!queryAnyEqual(shape, value));
		else
			assert(!queryAny(shape));
		maybeInsert(shape, value);
	}
	void remove(const auto& shape, const T& value)
	{
		assert(queryAnyEqual(shape, value));
		maybeRemove(shape, value);
	}
	void remove(const auto& shape)
	{
		assert(queryAny(shape));
		maybeRemove(shape);
	}
	void removeAll(const auto& shape, const T& value) { assert(queryAnyEqual(shape, value)); maybeRemove(shape, value); }
	void removeAll(const auto& shape) { assert(queryAny(shape)); maybeRemove(shape); }
	void prepare();
	void clear();
	[[nodiscard]] bool canPrepare() const;
	[[nodiscard]] __attribute__((noinline)) static T nullValue() { return T::create(nullPrimitive); }
	[[nodiscard]] bool empty() const { return leafCount() == 0; }
	[[nodiscard]] __attribute__((noinline)) bool anyLeafOverlapsAnother() const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] CuboidSet getLeafCuboids() const;
	[[nodiscard]] SmallSet<T> getAllWithCondition(auto&& condition) const
	{
		SmallSet<T> output;
		for(const Node& node : m_nodes)
		{
			const int leafCount = node.getLeafCount();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const RTreeArrayIndex end = RTreeArrayIndex::create(leafCount);
			for(RTreeArrayIndex i = {0}; i != end; ++i)
			{
				T value = T::create(nodeDataAndChildIndices[i].data);
				if(condition(value))
					output.maybeInsert(value);
			}
		}
		return output;
	}
	void load(const Json& data);
	void validate() const;
	void removeWithCondition(const auto& shape, const auto& condition)
	{
		const auto action = [](T& otherData) { otherData.clear(); };
		updateOrDestroyActionWithConditionAll(shape, action, condition);
	}
	void maybeRemoveWithConditionOne(const auto& shape, const auto& condition)
	{
		const auto action = [](T& otherData) { otherData.clear(); };
		maybeUpdateOrDestroyActionWithConditionOne(shape, action, condition);
	}
	[[nodiscard]] bool queryAnyEqual(const auto& shape, const T& value) const
	{
		const auto condition = [&](const T& v) { return value == v; };
		return queryAnyWithCondition(shape, condition);
	}
	struct UpdateActionConfig
	{
		bool create = false;
		bool destroy = false;
		bool stopAfterOne = false;
		bool allowNotFound = false;
		bool allowNotChanged = false;
		bool allowAlreadyExistsInAnotherSlot = false;
	};
	template<UpdateActionConfig queryConfig>
	void updateAction(const auto& shape, auto&& action)
	{
		// stopAfterOne is only safe to use for non-overlaping leaves, otherwise the result would be hard to predict.
		if(queryConfig.stopAfterOne)
			assert(!config.leavesCanOverlap && !config.splitAndMerge);
		auto condition = [](const T&){ return true; };
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	template<UpdateActionConfig queryConfig>
	void updateActionWithCondition(const CuboidSet& cuboidSet, auto&& action, const auto& condition)
	{
		for(const Cuboid& cuboid : cuboidSet)
			updateActionWithCondition<queryConfig>(cuboid, action, condition);
	}
	template<UpdateActionConfig queryConfig>
	void updateActionWithCondition(const auto& shape, auto&& action, const auto& condition)
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		bool found = false;
		// Track space which was not updated. If queryConfig.create is true then fill this space in at the end with the result of action(T::create(nullPrimitive)).
		[[maybe_unused]] CuboidSet emptySpaceInShape;
		if constexpr(queryConfig.create)
			emptySpaceInShape = CuboidSet::create(shape);
		// An action may cause a leaf to be shrunk or destroyed. Collect all nodes where this happens so we can correct their boundries.
		SmallSet<RTreeNodeIndex> toUpdateBoundryMaybe;
		// When a leaf is shrunk or destroyed it may generate fragments. Collect these to readd at end.
		MapWithCuboidKeys<T> fragmentsToReAdd;
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const int& leafCount = node.getLeafCount();
			BitSet intersectBitSet = BitSet::create(intersectMask);
			if(intersectMask.head(leafCount).any())
			{
				// Iterate the leaf segment of intersect mask.
				BitSet leafBitSet = intersectBitSet;
				if(leafCount != nodeSize)
					leafBitSet.clearAllAfterInclusive(leafCount);
				BitSet leavesToErase;
				while(leafBitSet.any())
				{
					const RTreeArrayIndex arrayIndex{leafBitSet.getNextAndClear()};
					Cuboid leafCuboid = nodeCuboids[arrayIndex.get()];
					// inflate from primitive temporarily for callback.
					T initalValue = T::create(nodeDataAndChildIndices[arrayIndex].data);
					assert(initalValue != T::create(nullPrimitive));
					T value = initalValue;
					if(!condition(value))
						continue;
					action(value);
					// Note space where action has been applied so we can create into empty space later.
					if constexpr(queryConfig.create)
						emptySpaceInShape.maybeRemove(leafCuboid.intersection(shape));
					found = true;
					if(value == initalValue)
					{
						// Action did not change the value.
						assert(queryConfig.allowNotChanged);
						continue;
					}
					if(shape.contains(leafCuboid))
					{
						if(value == T::create(nullPrimitive))
						{
							// Action changed value to null.
							assert(queryConfig.destroy);
							leavesToErase.set(arrayIndex.get());
							if(index != 0)
								toUpdateBoundryMaybe.maybeInsert(index);
						}
						else
						{
							// Update value without changing geometry.
							node.updateValue(arrayIndex, value);
						}
					}
					else
					{
						// Shape does not contain leaf. Record fragments and note index for boundry update.
						if(index != 0)
							toUpdateBoundryMaybe.maybeInsert(index);
						for(const Cuboid& cuboid : leafCuboid.getChildrenWhenSplitBy(shape))
							fragmentsToReAdd.insert(cuboid, initalValue);
						if(value == T::create(nullPrimitive))
						{
							// Action changed value to null.
							assert(queryConfig.destroy);
							leavesToErase.set(arrayIndex.get());
						}
						else
						{
							// Update leaf value and cuboid.
							node.updateLeafWithValue(arrayIndex, leafCuboid.intersection(shape),  value);
						}
					}
					if constexpr(queryConfig.stopAfterOne)
					{
						assert(leafCuboid.contains(shape));
						break;
					}
				}
				if constexpr(queryConfig.destroy)
					if(leavesToErase.any())
					{
						node.eraseLeavesByMask(leavesToErase);
					}
				//
				if constexpr(queryConfig.stopAfterOne)
					if(found)
						break;
			}
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(leafCount);
				addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
		// Repair any boundries altered by destroying or shrinking leaves.
		updateBoundriesMaybe(toUpdateBoundryMaybe);
		validate();
		// ReAdd fragments created by destroying or shrinking leaves.
		for(const auto& [cuboid, value] : fragmentsToReAdd)
			insert(cuboid, value);
		if constexpr(queryConfig.create)
		{
			// Create new leaves in the space within shape where no leaf currently exists.
			T value = T::create(nullPrimitive);
			action(value);
			assert(value != T::create(nullPrimitive));
			insert(emptySpaceInShape, value);
		}
		else if(!found && !queryConfig.allowNotFound)
		{
			// Nothing found to update
			assert(false);
			std::unreachable();
		}
	}
	// Change a value.
	void update(const auto& shape, const T& oldValue, const T& newValue)
	{
		const auto condition = [&](const T& value) { return value == oldValue; };
		const auto action = [&](T& value) { return value = newValue; };
		updateActionWithCondition<{}>(shape, action, condition);
	}
	void updateActionAll(const auto& shape, auto&& action)
	{
		constexpr UpdateActionConfig queryConfig{.allowNotFound = true};
		updateAction<queryConfig>(shape, action);
	}
	void updateOrCreateActionOne(const auto& shape, auto&& action)
	{
		constexpr UpdateActionConfig queryConfig{.create = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, action);
	}
	void updateOrDestroyActionOne(const auto& shape, auto&& action)
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, action);
	}
	void maybeUpdateOrDestroyActionAll(const auto& shape, auto&& action)
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .allowNotFound = true};
		updateAction<queryConfig>(shape, action);
	}
	void updateOrDestroyActionAll(const auto& shape, auto&& action)
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true};
		updateAction<queryConfig>(shape, action);
	}
	void updateActionWithConditionOne(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.stopAfterOne = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void updateActionWithConditionAll(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.allowNotFound = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void updateOrDestroyActionWithConditionAll(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .allowNotFound = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void updateOrCreateActionWithConditionOne(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.create = true, .stopAfterOne = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void updateOrCreateActionWithConditionAll(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.create = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void updateOrDestroyActionWithConditionOne(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void maybeUpdateOrDestroyActionWithConditionOne(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true, .allowNotFound = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	// Updates a value if it exists, creates one if not, destroys if the created value is equal to T::create(nullPrimitive).
	void updateOrCreateOrDestoryActionOne(const auto& shape, auto&& action)
	{
		constexpr UpdateActionConfig queryConfig{.create = true, .destroy = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, action);
	}
	void updateActionWithConditionOneAllowNotChanged(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{.stopAfterOne = true, .allowNotChanged = true};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	void updateActionWithConditionAllAllowNotChanged(const auto& shape, auto&& action, const auto& condition)
	{
		constexpr UpdateActionConfig queryConfig{};
		updateActionWithCondition<queryConfig>(shape, action, condition);
	}
	T updateAdd(const auto& shape, const T& value)
	{
		T output;
		constexpr UpdateActionConfig queryConfig{.create = true};
		updateAction<queryConfig>(shape, [&](T& v){ v += value; output = v; });
		return output;
	}
	T updateSubtract(const auto& shape, const T& value)
	{
		T output;
		constexpr UpdateActionConfig queryConfig{.destroy = true};
		updateAction<queryConfig>(shape, [&](T& v){ v -= value; output = v; });
		return output;
	}
	[[nodiscard]] bool queryAny(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(leafCount != 0 && intersectMask.head(leafCount).any())
				return true;
			if(node.hasChildren())
			{
				BitSet intersectBitSet = BitSet::create(intersectMask);
				intersectBitSet.clearAllBefore(leafCount);
				addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
		return false;
	}
	[[nodiscard]] bool queryAnyWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto leafCount = node.getLeafCount();
			BitSet intersectBitSet = BitSet::create(intersectMask);
			if(leafCount != 0)
			{
				BitSet leafBitSet = intersectBitSet;
				if(leafCount != nodeSize)
					leafBitSet.clearAllAfterInclusive(leafCount);
				while(leafBitSet.any())
				{
					const RTreeArrayIndex arrayIndex{leafBitSet.getNextAndClear()};
					if(condition(T::create(nodeDataAndChildIndices[arrayIndex].data)))
						return true;
				}
			}
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(leafCount);
				if(intersectBitSet.any())
					addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}

		}
		return false;
	}
	[[nodiscard]] bool queryAllWithCondition(const auto& shape, auto&& condition)
	{
		return queryGetAllCuboidsWithCondition(shape, condition).contains(shape);
	}
	[[nodiscard]] bool queryAll(const auto& shape)
	{
		auto condition = [&](const T&){ return true; };
		return queryAllWithCondition(shape, condition);
	}
	[[nodiscard]] const T queryGetFirst(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			BitSet intersectBitSet = BitSet::create(intersectMask);
			const RTreeArrayIndex arrayIndex{intersectBitSet.getNext()};
			if(arrayIndex < leafCount)
				return T::create(node.getDataAndChildIndices()[arrayIndex].data);
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(leafCount);
				if(intersectBitSet.any())
					addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
		return T::create(nullPrimitive);
	}
	[[nodiscard]] const T queryGetOne(const auto& shape) const
	{
		assert(!config.leavesCanOverlap);
		return queryGetFirst(shape);
	}
	[[nodiscard]] std::pair<T, Cuboid> queryGetOneWithCuboidAndCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			BitSet intersectBitSet = BitSet::create(intersectMask);
			if(leafCount != 0)
			{
				BitSet leafBitSet = intersectBitSet;
				if(offsetOfFirstChild != nodeSize)
					leafBitSet.clearAllAfterInclusive(offsetOfFirstChild.get());
				const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
				while(leafBitSet.any())
				{
					const RTreeArrayIndex arrayIndex{leafBitSet.getNextAndClear()};
					const auto candidate = T::create(nodeDataAndChildIndices[arrayIndex].data);
					if(condition(candidate))
						return {candidate, nodeCuboids[arrayIndex.get()]};
				}
			}
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(leafCount);
				addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
		return {T::create(nullPrimitive), Cuboid::null()};
	}
	[[nodiscard]] const T queryGetOneWithCondition(const auto& shape, const auto& condition) const
	{
		return queryGetOneWithCuboidAndCondition(shape, condition).first;
	}
	[[nodiscard]] Cuboid queryGetOneCuboidWithCondition(const auto& shape, const auto& condition) const
	{
		return queryGetOneWithCuboidAndCondition(shape, condition).second;
	}
	template<typename ShapeT>
	[[nodiscard]] Point3D queryGetOnePointWithCondition(const ShapeT& shape, const auto& condition) const
	{
		const Cuboid cuboid = queryGetOneCuboidWithCondition(shape, condition);
		return cuboid.exists() ? cuboid.intersectionPoint(shape) : Point3D::null();
	}
	[[nodiscard]] const T queryGetOneOr(const auto& shape, const T& alternate) const
	{
		const T& result = queryGetOne(shape);
		if(result == T::create(nullPrimitive))
			return alternate;
		return result;
	}
	[[nodiscard]] const std::pair<T, Cuboid> queryGetOneWithCuboid(const auto& shape) const
	{
		auto condition = [&](const T&){ return true; };
		return queryGetOneWithCuboidAndCondition(shape, condition);
	}
	void queryForEach(const auto& shape, auto&& action) const
	{
		auto wrappedAction = [&](const Cuboid&, const T& value) { action(value); };
		queryForEachWithCuboids(shape, wrappedAction);
	}
	void queryForEachCuboid(const auto& shape, auto&& action) const
	{
		auto wrappedAction = [&](const Cuboid& cuboid, const T&) { action(cuboid); };
		queryForEachWithCuboids(shape, wrappedAction);
	}
	void queryForEachWithCuboids(const auto& shape, auto&& action) const
	{
		SmallSet<RTreeNodeIndex> openList;
		SmallSet<T> output;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			BitSet intersectBitSet = BitSet::create(intersectMask);
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && intersectMask.head(leafCount).any())
			{
				BitSet intersectBitSetLeaves = intersectBitSet;
				if(leafCount != nodeSize)
					intersectBitSetLeaves.clearAllAfterInclusive(leafCount);
				while(intersectBitSetLeaves.any())
				{
					const RTreeArrayIndex arrayIndex{intersectBitSetLeaves.getNextAndClear()};
					action(nodeCuboids[arrayIndex.get()], T::create(nodeDataAndChildIndices[arrayIndex].data));
				}
			}
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(offsetOfFirstChild.get());
				addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
	}
	void forEachWithCuboids(auto&& action) const
	{
		const int nodeEnd = m_nodes.size();
		for(RTreeNodeIndex nodeIndex{0}; nodeIndex != nodeEnd; ++nodeIndex)
			// If empty slots were sorted this could be done in one pass.
			if(!m_emptySlots.contains(nodeIndex))
			{
				const Node& node = m_nodes[nodeIndex];
				const auto& cuboids = node.getCuboids();
				const auto& data = node.getDataAndChildIndices();
				const int leafEnd = node.getChildCount();
				for(RTreeArrayIndex arrayIndex{0}; arrayIndex != leafEnd; ++arrayIndex)
					action(cuboids[arrayIndex.get()], T::create(data[arrayIndex].data));
			}
	}
	[[nodiscard]] const SmallSet<T> queryGetAll(const auto& shape) const
	{
		SmallSet<T> output;
		auto action = [&](const T& value) { output.insertNonunique(value); };
		queryForEach(shape, action);
		output.makeUnique();
		return output;
	}
	[[nodiscard]] const SmallSet<std::pair<Cuboid, T>> queryGetAllWithCuboids(const auto& shape) const
	{
		SmallSet<std::pair<Cuboid, T>> output;
		auto action = [&](const Cuboid& cuboid, const T& value) { output.emplace(cuboid, value); };
		queryForEachWithCuboids(shape, action);
		return output;
	}
	[[nodiscard]] CuboidSet queryGetAllCuboids(const auto& shape) const
	{
		CuboidSet output;
		auto action = [&](const Cuboid& cuboid) { output.maybeAdd(cuboid); };
		queryForEachCuboid(shape, action);
		return output;
	}
	[[nodiscard]] CuboidSet queryGetAllCuboidsWithCondition(const auto& shape, const auto& condition) const
	{
		CuboidSet output;
		auto action = [&](const Cuboid& cuboid, const T& value) { if(condition(value)) output.maybeAdd(cuboid); };
		queryForEachWithCuboids(shape, action);
		return output;
	}
	[[nodiscard]] const SmallSet<T> queryGetAllWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<T> output;
		auto action = [&](const T& value) { if(condition(value)) output.insertNonunique(value); };
		queryForEach(shape, action);
		output.makeUnique();
		return output;
	}
	[[nodiscard]] const std::vector<std::pair<Cuboid, T>> queryGetAllWithCuboidsAndCondition(const auto& shape, const auto& condition) const
	{
		std::vector<std::pair<Cuboid, T>> output;
		auto action = [&](const Cuboid& cuboid, const T& value) { if(condition(value)) output.emplace_back(cuboid, value); };
		queryForEachWithCuboids(shape, action);
		std::ranges::sort(output);
		// TODO: std::ranges::unique
		output.erase(unique(output.begin(), output.end()), output.end());
		return output;
	}
	[[nodiscard]] bool batchQueryAny(const auto& shapes) const
	{
		std::vector<bool> output;
		output.resize(shapes.size());
		SmallMap<RTreeNodeIndex, SmallSet<int>> openList;
		SmallSet<int> topLevelCandidates;
		topLevelCandidates.resize(shapes.size());
		std::iota(topLevelCandidates.m_data.begin(), topLevelCandidates.m_data.end(), 0);
		openList.insert(RTreeNodeIndex::create(0), std::move(topLevelCandidates));
		while(!openList.empty())
		{
			const auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			for(const int& shapeIndex : candidates)
			{
				if(output[shapeIndex])
					// This shape has already intersected with a leaf, no need to check further.
					continue;
				const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
				// Record a leaf intersection.
				const auto leafCount = node.getLeafCount();
				if(leafCount != 0 && intersectMask.head(leafCount).any())
					return true;
				// Record intersected children in openList.
				BitSet intersectBitMask = BitSet::create(intersectMask);
				while(!intersectBitMask.empty())
				{
					const RTreeArrayIndex arrayIndex{intersectBitMask.getNextAndClear()};
					openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[arrayIndex].child)).insert(shapeIndex);
				}
			}
			// Pop back at the end so we can use candidates as a reference and not have to make a move-copy.
			openList.popBack();
		}
		return false;
	}
	[[nodiscard]] bool batchQueryWithConditionAny(const auto& shapes, const auto& condition) const
	{
		std::vector<bool> output;
		output.resize(shapes.size());
		SmallMap<RTreeNodeIndex, SmallSet<int>> openList;
		SmallSet<int> topLevelCandidates;
		topLevelCandidates.resize(shapes.size());
		std::iota(topLevelCandidates.m_data.begin(), topLevelCandidates.m_data.end(), 0);
		openList.insert(RTreeNodeIndex::create(0), std::move(topLevelCandidates));
		while(!openList.empty())
		{
			const auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			for(const int& shapeIndex : candidates)
			{
				if(output[shapeIndex])
					// This shape has already intersected with a leaf, no need to check further.
					continue;
				const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
				BitSet intersectBitMask = BitSet::create(intersectMask);
				BitSet leafBitMask = intersectBitMask;
				const auto leafCount = node.getLeafCount();
				leafBitMask.clearAllAfterInclusive(leafCount);
				const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
				while(!leafBitMask.empty())
				{
					const RTreeArrayIndex arrayIndex{leafBitMask.getNextAndClear()};
					if(condition(T::create(nodeDataAndChildIndices[arrayIndex].data)))
						return true;
				}
				// Record intersected children in openList.
				intersectBitMask.clearAllBefore(leafCount);
				while(!intersectBitMask.empty())
				{
					const RTreeArrayIndex arrayIndex{intersectBitMask.getNextAndClear()};
					openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[arrayIndex].child)).insert(shapeIndex);
				}
			}
			// Pop back at the end so we can use candidates as a reference and not have to make a move-copy.
			openList.popBack();
		}
		return false;
	}
	void batchQueryForEachWithCondition(const auto& shapes, auto&& action, auto&& condition) const
	{
		SmallMap<RTreeNodeIndex, SmallSet<int>> openList;
		SmallSet<int> topLevelCandidates;
		topLevelCandidates.resize(shapes.size());
		std::iota(topLevelCandidates.m_data.begin(), topLevelCandidates.m_data.end(), 0);
		openList.insert(RTreeNodeIndex::create(0), std::move(topLevelCandidates));
		while(!openList.empty())
		{
			const auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			for(const int& shapeIndex : candidates)
			{
				BitSet intersectMask = BitSet::create(nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]));
				BitSet leafMask = intersectMask;
				const int leafCount = node.getLeafCount();
				leafMask.clearAllAfterInclusive(leafCount);
				// Handle all leaf intersections.
				while(!leafMask.empty())
				{
					const RTreeArrayIndex arrayIndex{leafMask.getNextAndClear()};
					const T value = T::create(nodeDataAndChildIndices[arrayIndex].data);
					if(condition(value))
						action(value, nodeCuboids[arrayIndex.get()], shapeIndex);
				}
				// record all node intersections.
				intersectMask.clearAllBefore(leafCount);
				while(!intersectMask.empty())
				{
					const RTreeArrayIndex arrayIndex{intersectMask.getNextAndClear()};
					openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[arrayIndex].child)).insert(shapeIndex);
				}
			}
			openList.popBack();
		}
	}
	void batchQueryForEach(const auto& shapes, auto&& action) const
	{
		constexpr auto condition = [](const T&) { return true; };
		batchQueryForEachWithCondition(shapes, action, condition);
	}
	[[nodiscard]] std::vector<T> batchQueryGetFirstWithCondition(const auto& shapes, auto&& condition) const
	{
		std::vector<T> output;
		output.resize(shapes.size());
		auto action = [&](const T& value, const Cuboid&, const int& shapeIndex) mutable { assert(output[shapeIndex].empty()); output[shapeIndex] = value; };
		batchQueryForEachWithCondition(shapes, action, condition);
		return output;
	}
	[[nodiscard]] std::vector<SmallSet<T>> batchQueryGetAllWithCondition(const auto& shapes, auto&& condition) const
	{
		std::vector<SmallSet<T>> output;
		output.resize(shapes.size());
		auto action = [&](const T& value, const Cuboid&, const int& shapeIndex) mutable { assert(output[shapeIndex].empty()); output[shapeIndex].maybeInsert(value); };
		batchQueryForEachWithCondition(shapes, action, condition);
		return output;
	}
	[[nodiscard]] std::vector<CuboidSet> batchQueryGetAllCuboidsWithCondition(const auto& shapes, auto&& condition) const
	{
		std::vector<CuboidSet> output;
		output.resize(shapes.size());
		auto action = [&](const T&, const Cuboid& cuboid, const int& shapeIndex) mutable { assert(output[shapeIndex].empty()); output[shapeIndex].add(cuboid); };
		batchQueryForEachWithCondition(shapes, action, condition);
		return output;
	}
	[[nodiscard]] std::vector<std::pair<CuboidSet, T>> batchQueryGetAllWithCuboidsAndCondition(const auto& shapes, auto&& condition) const
	{
		std::vector<MapWithCuboidKeys<T>> output;
		output.resize(shapes.size());
		auto action = [&](const T& value, const Cuboid& cuboid, const int& shapeIndex) mutable { assert(output[shapeIndex].empty()); output[shapeIndex].insert({cuboid, value}); };
		batchQueryForEachWithCondition(shapes, action, condition);
		return output;
	}
	[[nodiscard]] std::vector<T> batchQueryGetFirst(const auto& shapes) const
	{
		constexpr auto condition = [](const T&) { return true; };
		return batchQueryGetFirstWithCondition(shapes, condition);
	}
	[[nodiscard]] std::vector<SmallSet<T>> batchQueryGetAll(const auto& shapes) const
	{
		constexpr auto condition = [](const T&) { return true; };
		return batchQueryGetAllWithCondition(shapes, condition);
	}
	[[nodiscard]] std::vector<CuboidSet> batchQueryGetAllCuboids(const auto& shapes) const
	{
		constexpr auto condition = [](const T&) { return true; };
		return batchQueryGetAllCuboidsWithCondition(shapes, condition);
	}
	[[nodiscard]] CuboidSet queryGetIntersection(const auto& shape) const
	{
		const CuboidSet leaves = queryGetAllCuboids(shape);
		CuboidSet output;
		for(const Cuboid& leaf : leaves)
			output.maybeAdd(leaf.intersection(shape));
		return output;
	}
	void queryRemove(CuboidSet& cuboids) const
	{
		Cuboid boundry = cuboids.boundry();
		for(const Cuboid& cuboid : queryGetAllCuboids(boundry))
			cuboids.maybeRemove(cuboid);
	}
	[[nodiscard]] int queryCount(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		int output = 0;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			output += intersectMask.head(leafCount).count();
			if(node.hasChildren())
			{
				BitSet intersectBitSet = BitSet::create(intersectMask);
				intersectBitSet.clearAllBefore(leafCount);
				if(intersectBitSet.any())
					addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
		return output;
	}
	[[nodiscard]] int queryCountWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		int output = 0;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			BitSet intersectBitSet = BitSet::create(intersectMask);
			const auto leafCount = node.getLeafCount();
			const auto offsetOfFirstChild = node.offsetOfFirstChild();
			if(leafCount != 0 && intersectMask.head(leafCount).any())
			{
				const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
				BitSet leafBitSet = intersectBitSet;
				if(offsetOfFirstChild != nodeSize)
					leafBitSet.clearAllAfterInclusive(offsetOfFirstChild.get());
				while(leafBitSet.any())
				{
					const RTreeArrayIndex arrayIndex{leafBitSet.getNextAndClear()};
					T value = T::create(nodeDataAndChildIndices[arrayIndex].data);
					if(condition(value))
						++output;
				}
			}
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(leafCount);
				addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}
		}
		return output;
	}
	template<typename Result, auto predicate>
	[[nodiscard]] std::pair<T, Result> queryGetHighestReturnWithPredicateOutput(const auto& shape) const
	{
		T outputValue;
		Result outputResult;
		bool first = true;
		auto action = [&](const T& value)
		{
			Result result = predicate(value);
			if(first || result > outputResult)
			{
				first = false;
				outputResult = result;
				outputValue = value;
			}
		};
		queryForEach(shape, action);
		return {outputValue, outputResult};
	}
	[[nodiscard]] T queryGetLowest(const auto& shape) const
	{
		T output;
		bool first = true;
		auto action = [&](const T& value)
		{
			if(first || value < output)
			{
				first = false;
				output = value;
			}
		};
		queryForEach(shape, action);
		return output;
	}
	[[nodiscard]] Point3D queryGetLowestPoint(const auto& shape) const
	{
		Point3D output;
		T outputValue;
		bool first = true;
		auto action = [&](const Cuboid& cuboid, const T& value)
		{
			if(first || value < outputValue)
			{
				first = false;
				output = cuboid.intersectionPoint(shape);
				outputValue = value;
			}
		};
		queryForEach(shape, action);
		return output;
	}
	[[nodiscard]] T queryNearestWithCondition(const auto& shape, const Point3D& location, auto&& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		Distance closestDistance = Distance::max();
		T output;
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!intersectMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto leafCount = node.getLeafCount();
			BitSet intersectBitSet = BitSet::create(intersectMask);
			if(leafCount != 0)
			{
				BitSet leafBitSet = intersectBitSet;
				if(leafCount != nodeSize)
					leafBitSet.clearAllAfterInclusive(leafCount);
				while(leafBitSet.any())
				{
					const RTreeArrayIndex arrayIndex{leafBitSet.getNextAndClear()};
					// Using center distance because it's much cheaper to compute then edge distance.
					const Distance distance = nodeCuboids[arrayIndex.get()].center().distanceTo(location);
					if(distance >= closestDistance)
						continue;
					const auto candidate = T::create(nodeDataAndChildIndices[arrayIndex].data);
					if(condition(candidate))
					{
						closestDistance = distance;
						output = candidate;
					}
				}
			}
			if(node.hasChildren())
			{
				intersectBitSet.clearAllBefore(leafCount);
				if(intersectBitSet.any())
					addIntersectedChildrenToOpenList(node, intersectBitSet, openList);
			}

		}
		return output;
	}
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) int nodeCount() const;
	[[nodiscard]] __attribute__((noinline)) int leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(int i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(int i, int o) const;
	[[nodiscard]] __attribute__((noinline)) RTreeNodeIndex getNodeChild(int i, int o) const;
	[[nodiscard]] __attribute__((noinline)) T queryPointOne(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) T queryPointFirst(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) SmallSet<T> queryPointAll(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(int x, int y, int z, const T& value) const;
	[[nodiscard]] __attribute__((noinline)) int queryPointCount(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) Cuboid queryPointCuboid(int x, int y, int z) const;
	[[nodiscard]] __attribute__((noinline)) int totalLeafVolume() const;
	[[nodiscard]] __attribute__((noinline)) int totalNodeVolume() const;
	__attribute__((noinline)) void assertAllLeafsAreUnique() const;
	[[nodiscard]] static __attribute__((noinline)) int getNodeSize();
	__attribute__((noinline)) std::string toString(int x, int y, int z) const;
};
template<Sortable T, RTreeDataConfig config>
void to_json(Json& data, const RTreeData<T, config>& tree) { data = tree.toJson(); }
template<Sortable T, RTreeDataConfig config>
void from_json(const Json& data, RTreeData<T, config>& tree) { tree.load(data); }

// Wraps data to have the parts that RTreeData expects, such as ::Primitive.
template<typename T, T defaultValue>
struct RTreeDataWrapper
{
	T data = defaultValue;
	using Primitive = T;
	void clear() { data = defaultValue; }
	[[nodiscard]] constexpr T get() const { return data; }
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const RTreeDataWrapper<T, defaultValue>& other) const = default;
	[[nodiscard]] constexpr bool operator==(const RTreeDataWrapper<T, defaultValue>& other) const = default;
	[[nodiscard]] constexpr bool exists() const { return data != defaultValue; }
	[[nodiscard]] constexpr bool empty() const { return data == defaultValue; }
	[[nodiscard]] constexpr static RTreeDataWrapper<T, defaultValue> create(const T& p) { return {p}; }
	[[nodiscard]] constexpr static RTreeDataWrapper<T, defaultValue> null() { return {defaultValue}; }
	[[nodiscard]] constexpr static Primitive nullPrimitive() { return defaultValue; }
};