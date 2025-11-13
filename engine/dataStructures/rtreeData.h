#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/mapWithCuboidKeys.h"
#include "../strongInteger.h"
#include "../json.h"
#include "../numericTypes/index.h"
#include "../concepts.h"
#include "strongVector.hpp"
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
	static constexpr uint8_t nodeSize = 64;
	union DataOrChild { T::Primitive data; RTreeNodeIndex::Primitive child; };
	class Node
	{
		CuboidArray<nodeSize> m_cuboids;
		std::array<DataOrChild, nodeSize> m_dataAndChildIndices;
		RTreeNodeIndex m_parent;
		RTreeArrayIndex m_leafEnd = RTreeArrayIndex::create(0);
		RTreeArrayIndex m_childBegin = RTreeArrayIndex::create(nodeSize);
		// TODO: store sort order?
	public:
		[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
		[[nodiscard]] const auto& getDataAndChildIndices() const { return m_dataAndChildIndices; }
		[[nodiscard]] auto& getDataAndChildIndices() { return m_dataAndChildIndices; }
		[[nodiscard]] const auto& getParent() const { return m_parent; }
		[[nodiscard]] uint8_t getLeafCount() const { return m_leafEnd.get(); }
		[[nodiscard]] uint8_t getChildCount() const { return nodeSize - m_childBegin.get(); }
		[[nodiscard]] uint8_t unusedCapacity() const { return (m_childBegin - m_leafEnd).get(); }
		[[nodiscard]] int sortOrder() const { return m_cuboids.boundry().getCenter().hilbertNumber(); };
		[[nodiscard]] RTreeArrayIndex offsetFor(const RTreeNodeIndex& index) const;
		[[nodiscard]] RTreeArrayIndex offsetOfFirstChild() const { return m_childBegin; }
		[[nodiscard]] bool empty() const { return unusedCapacity() == nodeSize; }
		[[nodiscard]] unsigned int getLeafVolume() const;
		[[nodiscard]] unsigned int getNodeVolume() const;
		[[nodiscard]] Json toJson() const;
		[[nodiscard]] bool containsLeaf(const Cuboid& cuboid, const T& value);
		[[nodiscard]] bool operator==(const Node& other) const = default;
		void load(const Json& data);
		void updateChildIndex(const RTreeNodeIndex& oldIndex, const RTreeNodeIndex& newIndex);
		void insertLeaf(const Cuboid& cuboid, const T& value);
		void insertLeaf(const Point3D& point, const T& value) { insertLeaf(Cuboid{point, point}, value); }
		void insertBranch(const Cuboid& cuboid, const RTreeNodeIndex& index);
		void eraseBranch(const RTreeArrayIndex& offset);
		void eraseLeaf(const RTreeArrayIndex& offset);
		void eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
		void eraseLeavesByMask(const BitSet<uint64_t, nodeSize>& mask);
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
	// Removes intercepting leaves and contained branches. Intercepting branches are added to openList.
	void removeFromNode(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList);
	void removeFromNodeWithValue(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList, const T& value);
	void removeFromNodeByMask(Node& node, const Eigen::Array<bool, 1, Eigen::Dynamic>& mask);
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
	static void addIntersectedChildrenToOpenList(const Node& node, const CuboidArray<nodeSize>::BoolArray& intersecting, SmallSet<RTreeNodeIndex>& openList);
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
	[[nodiscard]] __attribute__((noinline)) static T nullValue() { return T::create(nullPrimitive); }
	[[nodiscard]] bool empty() const { return leafCount() == 0; }
	[[nodiscard]] __attribute__((noinline)) bool anyLeafOverlapsAnother() const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] CuboidSet getLeafCuboids() const;
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const uint8_t& leafCount = node.getLeafCount();
			if(interceptMask.head(leafCount).any())
			{
				// Iterate the leaf segment of intercept mask.
				const auto begin = interceptMask.begin();
				const auto leafEnd = begin + leafCount;
				BitSet<uint64_t, nodeSize> leavesToErase;
				for(auto iter = begin; iter != leafEnd; ++iter)
				{
					if(*iter)
					{
						uint8_t offset = iter - begin;
						Cuboid leafCuboid = nodeCuboids[offset];
						// inflate from primitive temporarily for callback.
						T initalValue = T::create(nodeDataAndChildIndices[offset].data);
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
								leavesToErase.set(offset);
								if(index != 0)
									toUpdateBoundryMaybe.maybeInsert(index);
							}
							else
							{
								// Update value without changing geometry.
								node.updateValue(RTreeArrayIndex::create(offset), value);
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
								leavesToErase.set(offset);
							}
							else
							{
								// Update leaf value and cuboid.
								node.updateLeafWithValue(RTreeArrayIndex::create(offset), leafCuboid.intersection(shape),  value);
							}
						}
						if constexpr(queryConfig.stopAfterOne)
						{
							assert(leafCuboid.contains(shape));
							break;
						}
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
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
				return true;
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto leafCount = node.getLeafCount();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				const auto begin = interceptMask.begin();
				const auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const uint8_t offset = iter - begin;
						if(condition(T::create(nodeDataAndChildIndices[offset].data)))
							return true;
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);

		}
		return false;
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						return T::create(nodeDataAndChildIndices[iter - begin].data);
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return T::create(nullPrimitive);
	}
	[[nodiscard]] const T queryGetOne(const auto& shape) const
	{
		assert(!config.leavesCanOverlap);
		return queryGetFirst(shape);
	}
	[[nodiscard]] const T queryGetOneWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto candidate = T::create(nodeDataAndChildIndices[iter - begin].data);
						if(condition(candidate))
							return candidate;
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return T::create(nullPrimitive);
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						const auto candidate = T::create(nodeDataAndChildIndices[offset].data);
						if(condition(candidate))
							return {candidate, nodeCuboids[offset]};
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return {T::create(nullPrimitive), Cuboid::null()};
	}
	[[nodiscard]] Cuboid queryGetOneCuboidWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						const auto candidate = T::create(nodeDataAndChildIndices[offset].data);
						if(condition(candidate))
							return nodeCuboids[offset];
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return Cuboid::null();
	}
	template<typename ShapeT>
	[[nodiscard]] Point3D queryGetOnePointWithCondition(const ShapeT& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						const auto candidate = T::create(nodeDataAndChildIndices[offset].data);
						if(condition(candidate))
						{
							if constexpr(std::is_same_v<CuboidSet, ShapeT>)
								return nodeCuboids[offset].intersection(shape).front().m_high;
							else
								return nodeCuboids[offset].intersection(shape).m_high;
						}
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return Point3D::null();
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
		assert(!config.leavesCanOverlap);
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						return {T::create(nodeDataAndChildIndices[offset].data), nodeCuboids[offset]};
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return {T::create(nullPrimitive), {}};
	}
	[[nodiscard]] const SmallSet<T> queryGetAll(const auto& shape) const
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
						output.insertNonunique(T::create(nodeDataAndChildIndices[iter - begin].data));
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		output.makeUnique();
		return output;
	}
	[[nodiscard]] const SmallSet<std::pair<Cuboid, T>> queryGetAllWithCuboids(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		SmallSet<std::pair<Cuboid, T>> output;
		openList.insert(RTreeNodeIndex::create(0));
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
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	[[nodiscard]] CuboidSet queryGetAllCuboids(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		CuboidSet output;
		openList.insert(RTreeNodeIndex::create(0));
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						output.maybeAdd(nodeCuboids[offset]);
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	[[nodiscard]] CuboidSet queryGetAllCuboidsWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		CuboidSet output;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						const T value = T::create(nodeDataAndChildIndices[offset].data);
						if(condition(value))
							output.maybeAdd(nodeCuboids[offset]);
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	[[nodiscard]] const SmallSet<T> queryGetAllWithCondition(const auto& shape, const auto& condition) const
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
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			const auto leafCount = node.getLeafCount();
			if(!interceptMask.any())
				continue;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						T value = T::create(nodeDataAndChildIndices[iter - begin].data);
						if(condition(value))
							output.insertNonunique(value);
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		output.makeUnique();
		return output;
	}
	[[nodiscard]] const std::vector<std::pair<Cuboid, T>> queryGetAllWithCuboidsAndCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		std::vector<std::pair<Cuboid, T>> output;
		openList.insert(RTreeNodeIndex::create(0));
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
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						T value = T::create(nodeDataAndChildIndices[offset].data);
						if(condition(value))
							output.emplace_back(nodeCuboids[offset], value);
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		std::ranges::sort(output);
		output.erase(unique(output.begin(), output.end()), output.end());
		return output;
	}
	[[nodiscard]] bool batchQueryAny(const auto& shapes) const
	{
		assert(shapes.size() < UINT8_MAX);
		std::vector<bool> output;
		output.resize(shapes.size());
		SmallMap<RTreeNodeIndex, SmallSet<uint8_t>> openList;
		openList.insert(RTreeNodeIndex::create(0), {});
		auto& rootNodeCandidateList = openList.back().second;
		rootNodeCandidateList.resize(shapes.size());
		std::ranges::iota(rootNodeCandidateList.getVector(), 0);
		while(!openList.empty())
		{
			const auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			for(const uint8_t& shapeIndex : candidates)
			{
				if(output[shapeIndex])
					// This shape has already intersected with a leaf, no need to check further.
					continue;
				const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
				// Record a leaf intersection.
				const auto leafCount = node.getLeafCount();
				if(leafCount != 0 && interceptMask.head(leafCount).any())
					return true;
				// Record intersected children in openList.
				const auto offsetOfFirstChild = node.offsetOfFirstChild();
				const uint8_t childCount = nodeSize - offsetOfFirstChild.get();
				if(childCount == 0 || !interceptMask.tail(childCount).any())
					continue;
				const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
				const auto begin = interceptMask.begin();
				const auto end = interceptMask.end();
				if(offsetOfFirstChild == 0)
				{
					// Unrollable version for nodes where every slot is a child.
					for(auto iter = begin; iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[iter - begin].child)).insert(shapeIndex);
				}
				else
				{
					for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[iter - begin].child)).insert(shapeIndex);
				}
			}
			// Pop back at the end so we can use candidates as a reference and not have to make a move-copy.
			openList.popBack();
		}
		return false;
	}
	[[nodiscard]] bool batchQueryWithConditionAny(const auto& shapes, const auto& condition) const
	{
		assert(shapes.size() < UINT8_MAX);
		std::vector<bool> output;
		output.resize(shapes.size());
		SmallMap<RTreeNodeIndex, SmallSet<uint8_t>> openList;
		openList.insert(RTreeNodeIndex::create(0), {});
		auto& rootNodeCandidateList = openList.back().second;
		rootNodeCandidateList.resize(shapes.size());
		std::ranges::iota(rootNodeCandidateList.getVector(), 0);
		while(!openList.empty())
		{
			const auto& [index, candidates] = openList.back();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			for(const uint8_t& shapeIndex : candidates)
			{
				if(output[shapeIndex])
					// This shape has already intersected with a leaf, no need to check further.
					continue;
				const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
				// Record a leaf intersection.
				const auto leafCount = node.getLeafCount();
				const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
				const auto begin = interceptMask.begin();
				const auto end = interceptMask.end();
				const auto leafEnd = begin + leafCount;
				if(leafCount != 0 && interceptMask.head(leafCount).any())
				{
					for(auto iter = begin; iter != leafEnd; ++iter)
						if(*iter && condition(T::create(nodeDataAndChildIndices[iter - begin].data)))
							return true;
				}
				// Record intersected children in openList.
				const auto offsetOfFirstChild = node.offsetOfFirstChild();
				const uint8_t childCount = nodeSize - offsetOfFirstChild.get();
				if(childCount == 0 || !interceptMask.tail(childCount).any())
					continue;
				if(offsetOfFirstChild == 0)
				{
					// Unrollable version for nodes where every slot is a child.
					for(auto iter = begin; iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[iter - begin].child)).insert(shapeIndex);
				}
				else
				{
					for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
						if(*iter)
							openList.getOrCreate(RTreeNodeIndex::create(nodeDataAndChildIndices[iter - begin].child)).insert(shapeIndex);
				}
			}
			// Pop back at the end so we can use candidates as a reference and not have to make a move-copy.
			openList.popBack();
		}
		return false;
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
	[[nodiscard]] uint16_t queryCount(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		uint16_t output = 0;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!interceptMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			output += interceptMask.head(leafCount).count();
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	[[nodiscard]] uint16_t queryCountWithCondition(const auto& shape, const auto& condition) const
	{
		SmallSet<RTreeNodeIndex> openList;
		uint16_t output = 0;
		openList.insert(RTreeNodeIndex::create(0));
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!interceptMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						T value = T::create(nodeDataAndChildIndices[iter - begin].data);
						if(condition(value))
							output += interceptMask.head(leafCount).count();
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	template<typename Result, auto predicate>
	[[nodiscard]] std::pair<T, Result> queryGetHighestReturnWithPredicateOutput(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		T outputValue;
		Result outputResult;
		openList.insert(RTreeNodeIndex::create(0));
		bool first = true;;
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!interceptMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						T value = T::create(nodeDataAndChildIndices[iter - begin].data);
						Result result = predicate(value);
						if(first || result > outputResult)
						{
							first = false;
							outputResult = result;
							outputValue = value;
						}
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return {outputValue, outputResult};
	}
	[[nodiscard]] T queryGetLowest(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		T output;
		openList.insert(RTreeNodeIndex::create(0));
		bool first = true;;
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!interceptMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						T value = T::create(nodeDataAndChildIndices[offset].data);
						if(first || value < output)
						{
							first = false;
							output = value;
						}
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	[[nodiscard]] Point3D queryGetLowestPoint(const auto& shape) const
	{
		SmallSet<RTreeNodeIndex> openList;
		Point3D output;
		T outputValue;
		openList.insert(RTreeNodeIndex::create(0));
		bool first = true;;
		while(!openList.empty())
		{
			auto index = openList.back();
			openList.popBack();
			const Node& node = m_nodes[index];
			const auto& nodeCuboids = node.getCuboids();
			const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
			if(!interceptMask.any())
				continue;
			const auto leafCount = node.getLeafCount();
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			if(leafCount != 0 && interceptMask.head(leafCount).any())
			{
				auto begin = interceptMask.begin();
				auto end = begin + leafCount;
				for(auto iter = begin; iter != end; ++iter)
					if(*iter)
					{
						const auto offset = iter - begin;
						T value = T::create(nodeDataAndChildIndices[offset].data);
						if(first || value < outputValue)
						{
							first = false;
							output = nodeCuboids[offset].intersectionPoint(shape);
							outputValue = value;
						}
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		return output;
	}
	// For test and debug.
	[[nodiscard]] __attribute__((noinline)) uint32_t nodeCount() const;
	[[nodiscard]] __attribute__((noinline)) uint32_t leafCount() const;
	[[nodiscard]] __attribute__((noinline)) const Node& getNode(uint i) const;
	[[nodiscard]] __attribute__((noinline)) const Cuboid getNodeCuboid(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) RTreeNodeIndex getNodeChild(uint i, uint o) const;
	[[nodiscard]] __attribute__((noinline)) T queryPointOne(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) T queryPointFirst(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) SmallSet<T> queryPointAll(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) bool queryPoint(uint x, uint y, uint z, const T& value) const;
	[[nodiscard]] __attribute__((noinline)) uint8_t queryPointCount(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) Cuboid queryPointCuboid(uint x, uint y, uint z) const;
	[[nodiscard]] __attribute__((noinline)) uint totalLeafVolume() const;
	[[nodiscard]] __attribute__((noinline)) uint totalNodeVolume() const;
	__attribute__((noinline)) void assertAllLeafsAreUnique() const;
	[[nodiscard]] static __attribute__((noinline)) uint getNodeSize();
	__attribute__((noinline)) std::string toString(uint x, uint y, uint z) const;
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