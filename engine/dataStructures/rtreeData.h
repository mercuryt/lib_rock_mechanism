#pragma once
#include "../geometry/cuboid.h"
#include "../geometry/cuboidArray.h"
#include "../geometry/cuboidSet.h"
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

template<Sortable T, RTreeDataConfig config = RTreeDataConfig()>
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
	T m_nullValue;
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
	void insert(const auto& shape, const T& value) { assert(!queryAnyEqual(shape, value)); maybeInsert(shape, value); }
	void remove(const auto& shape, const T& value) { assert(queryAnyEqual(shape, value)); maybeRemove(shape, value); }
	void removeAll(const auto& shape, const T& value) { assert(queryAnyEqual(shape, value)); maybeRemove(shape, value); }
	void removeAll(const auto& shape) { assert(queryAny(shape)); maybeRemove(shape); }
	void prepare();
	void clear();
	[[nodiscard]] bool empty() const { return leafCount() == 0; }
	[[nodiscard]] __attribute__((noinline)) bool anyLeafOverlapsAnother();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] CuboidSet getLeafCuboids() const;
	void load(const Json& data);
	void maybeRemoveWithConditionOne(const auto& shape, const auto& condition)
	{
		const auto action = [](T& otherData) { otherData.clear(); };
		maybeUpdateOrDestroyActionWithConditionOne(shape, action, condition);
	}
	bool queryAnyEqual(const auto& shape, const T& value) const
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
		bool applyToEmptySpace = false;
	};
	template<UpdateActionConfig queryConfig>
	void updateAction(const auto& shape, auto&& action, const T& nullValue = T())
	{
		// stopAfterOne is only safe to use for non-overlaping leaves, otherwise the result would be hard to predict.
		if(queryConfig.stopAfterOne)
			assert(!config.leavesCanOverlap);
		// Both create and applyToEmpty would be redundant.
		assert(queryConfig.applyToEmptySpace + queryConfig.create < 2);
		[[maybe_unused]] CuboidSet emptySpaceInShape = CuboidSet::create(shape.boundry());
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		bool updated = false;
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
			const auto& leafCount = node.getLeafCount();
			const auto& begin = interceptMask.begin();
			const auto leafEnd = begin + leafCount;
			// Erasing leafs durring iteration would invalidate the intercept mask. Collect and then erase all at end.
			BitSet<uint64_t, nodeSize> leavesToErase;
			if(interceptMask.head(leafCount).any())
			{
				for(auto iter = begin; iter != leafEnd; ++iter)
					if(*iter)
					{
						uint8_t offset = iter - begin;
						Cuboid leafCuboid = nodeCuboids[offset];
						if constexpr(queryConfig.applyToEmptySpace)
							emptySpaceInShape.remove(leafCuboid.intersection(shape));
						// inflate from primitive temporarily for callback.
						T initalValue = T::create(nodeDataAndChildIndices[offset].data);
						assert(initalValue != nullValue && initalValue != m_nullValue);
						T value = initalValue;
						action(value);
						if constexpr(queryConfig.allowAlreadyExistsInAnotherSlot)
						{
							if(node.containsLeaf(leafCuboid, value))
								return;
						}
						else
							assert(!node.containsLeaf(leafCuboid, value));
						// When nullValue is specificed it will probably always be 0.
						// This is distinct from m_nullValue which will usually be the largest integer T can store.
						// If nullValue is not provided these checks become redundant and the compiler will remove one.
						if(value == nullValue || value == m_nullValue)
						{
							// Destroy updated leaf.
							assert(queryConfig.destroy);
							if constexpr(queryConfig.stopAfterOne)
								node.eraseLeaf(RTreeArrayIndex::create(offset));
							else
								leavesToErase.set(offset);
						}
						else
						{
							// Update leaf.
							const Cuboid intersection = leafCuboid.intersection((shape));
							if constexpr(queryConfig.allowNotChanged)
								node.maybeUpdateLeafWithValue(RTreeArrayIndex::create(offset), intersection, value);
							else
								node.updateLeafWithValue(RTreeArrayIndex::create(offset), intersection, value);
							// Validate any overlaps.
							if constexpr(!config.leavesCanOverlap)
								assert(queryCount(intersection) == 1);
							else
							{
								const auto valuesAndCuboids = queryGetAllWithCuboids(intersection);
								bool newlyCreatedFound = false;
								for(const auto& [cuboid, v] : valuesAndCuboids)
								{
									if(cuboid == intersection && v == value)
									{
										assert(!newlyCreatedFound);
										newlyCreatedFound = true;
										continue;
									}
									assert(canOverlap(v, value));
								}
							}
						}
						if(!shape.contains(leafCuboid))
						{
							// The leaf cuboid is intercepted but not contained, fragment the unintercepted parts and re add them with the inital value.
							assert(config.splitAndMerge);
							const auto fragments = leafCuboid.getChildrenWhenSplitBy(shape);
							// Re-add fragments.
							const auto fragmentsEnd = fragments.end();
							for(auto i = fragments.begin(); i != fragmentsEnd; ++i)
								addToNodeRecursive(index, *i, initalValue);
						}
						if constexpr(queryConfig.stopAfterOne)
							return;
						else
							updated = true;
					}
				if constexpr(queryConfig.destroy)
					if(leavesToErase.any())
					{
						assert(!queryConfig.stopAfterOne);
						node.eraseLeavesByMask(leavesToErase);
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		if(!updated)
		{
			if constexpr(queryConfig.create)
			{
				T value = nullValue;
				action(value);
				assert(value != m_nullValue && value != nullValue);
				maybeInsert(shape, value);
			}
			else if constexpr(queryConfig.applyToEmptySpace)
			{
				T value = nullValue;
				action(value);
				assert(value != m_nullValue && value != nullValue);
				for(const Cuboid& cuboid : emptySpaceInShape)
					maybeInsert(cuboid, value);
			}
			else if constexpr(!queryConfig.allowNotFound)
			{
				// Nothing found to update
				assert(false);
				std::unreachable();
			}
		}
	}
	template<UpdateActionConfig queryConfig>
	void updateActionWithCondition(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		SmallSet<RTreeNodeIndex> openList;
		openList.insert(RTreeNodeIndex::create(0));
		bool updated = false;
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
				const auto begin = interceptMask.begin();
				const auto leafEnd = begin + leafCount;
				BitSet<uint64_t, nodeSize> leavesToErase;
				for(auto iter = begin; iter != leafEnd; ++iter)
					if(*iter)
					{
						uint8_t offset = iter - begin;
						Cuboid leafCuboid = nodeCuboids[offset];
						// inflate from primitive temporarily for callback.
						T initalValue = T::create(nodeDataAndChildIndices[offset].data);
						assert(initalValue != nullValue && initalValue != m_nullValue);
						T value = initalValue;
						if(!condition(value))
							continue;
						action(value);
						// When nullValue is specificed it will probably always be 0.
						// This is distinct from m_nullValue which will usually be the largest integer T can store.
						// If nullValue is not provided these checks become redundant and the compiler will remove one.
						if(value == nullValue || value == m_nullValue)
						{
							assert(queryConfig.destroy);
							if constexpr (queryConfig.stopAfterOne)
								node.eraseLeaf(RTreeArrayIndex::create(offset));
							else
								leavesToErase.set(offset);
						}
						else
						{
							Cuboid intersection = leafCuboid.intersection((shape));
							if constexpr(queryConfig.allowNotChanged)
								node.maybeUpdateLeafWithValue(RTreeArrayIndex::create(offset), intersection, value);
							else
								node.updateLeafWithValue(RTreeArrayIndex::create(offset), intersection, value);
							// Validate any overlaps.
							if constexpr(!config.leavesCanOverlap)
								assert(queryCount(intersection) == 1);
							else
							{
								const auto valuesAndCuboids = queryGetAllWithCuboids(intersection);
								bool newlyCreatedFound = false;
								for(const auto& [cuboid, v] : valuesAndCuboids)
								{
									if(cuboid == intersection && v == value)
									{
										assert(!newlyCreatedFound);
										newlyCreatedFound = true;
										continue;
									}
									assert(canOverlap(v, value));
								}
							}
						}
						if(!shape.contains(leafCuboid))
						{
							const auto fragments = leafCuboid.getChildrenWhenSplitBy(shape);
							// Re-add fragments.
							const auto fragmentsEnd = fragments.end();
							for(auto i = fragments.begin(); i != fragmentsEnd; ++i)
								addToNodeRecursive(index, *i, initalValue);
						}
						if constexpr(queryConfig.stopAfterOne)
							return;
						else
							updated = true;
					}
				if constexpr(queryConfig.destroy)
					if(leavesToErase.any())
					{
						assert(!queryConfig.stopAfterOne);
						node.eraseLeavesByMask(leavesToErase);
					}
			}
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
		}
		if(!updated)
		{
			if constexpr(queryConfig.create)
			{
				T value = nullValue;
				action(value);
				assert(value != m_nullValue && value != nullValue);
				maybeInsert(shape, value);
			}
			else if constexpr (!queryConfig.allowNotFound)
			{
				// Nothing found to update
				assert(false);
				std::unreachable();
			}
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
	void updateOrCreateActionOne(const auto& shape, auto&& action, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.create = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, action, nullValue);
	}
	void updateOrDestroyActionOne(const auto& shape, auto&& action, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, action, nullValue);
	}
	void maybeUpdateOrDestroyActionAll(const auto& shape, auto&& action, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .allowNotFound = true};
		updateAction<queryConfig>(shape, action, nullValue);
	}
	void updateOrDestroyActionAll(const auto& shape, auto&& action, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true};
		updateAction<queryConfig>(shape, action, nullValue);
	}
	void updateActionWithConditionOne(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.stopAfterOne = true};
		updateActionWithCondition<queryConfig>(shape, action, condition, nullValue);
	}
	void updateActionWithConditionAll(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.allowNotFound = true};
		updateActionWithCondition<queryConfig>(shape, action, condition, nullValue);
	}
	void updateOrCreateActionWithConditionOne(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.create = true, .stopAfterOne = true};
		updateActionWithCondition<queryConfig>(shape, action, condition, nullValue);
	}
	void updateOrDestroyActionWithConditionOne(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true};
		updateActionWithCondition<queryConfig>(shape, action, condition, nullValue);
	}
	void maybeUpdateOrDestroyActionWithConditionOne(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true, .allowNotFound = true};
		updateActionWithCondition<queryConfig>(shape, action, condition, nullValue);
	}
	// Updates a value if it exists, creates one if not, destroys if the created value is equal to nullValue or m_nullValue.
	void updateOrCreateOrDestoryActionOne(const auto& shape, auto&& action, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.create = true, .destroy = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, action, nullValue);
	}
	void updateActionWithConditionOneAllowNotChanged(const auto& shape, auto&& action, const auto& condition, const T& nullValue = T())
	{
		constexpr UpdateActionConfig queryConfig{.stopAfterOne = true, .allowNotChanged = true};
		updateActionWithCondition<queryConfig>(shape, action, condition, nullValue);
	}
	T updateAddOne(const auto& shape, const T& value)
	{
		T output;
		constexpr UpdateActionConfig queryConfig{.stopAfterOne = true, .applyToEmptySpace = true};
		updateAction<queryConfig>(shape, [&](T& v){ v += value; output = v; }, T::create(0));
		return output;
	}
	T updateSubtractOne(const auto& shape, const T& value)
	{
		T output;
		constexpr UpdateActionConfig queryConfig{.destroy = true, .stopAfterOne = true};
		updateAction<queryConfig>(shape, [&](T& v){ v -= value; output = v; }, T::create(0));
		return output;
	}
	T updateAddAll(const auto& shape, const T& value)
	{
		T output;
		constexpr UpdateActionConfig queryConfig{.applyToEmptySpace = true};
		updateAction<queryConfig>(shape, [&](T& v){ v += value; output = v; }, T::create(0));
		return output;
	}
	T updateSubtractAll(const auto& shape, const T& value)
	{
		T output;
		constexpr UpdateActionConfig queryConfig{.destroy = true};
		updateAction<queryConfig>(shape, [&](T& v){ v -= value; output = v; }, T::create(0));
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
		return m_nullValue;
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
		return m_nullValue;
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
		return {m_nullValue, Cuboid::null()};
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
		if(result == m_nullValue)
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
		return {m_nullValue, {}};
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
						output.add(nodeCuboids[offset]);
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
							output.add(nodeCuboids[offset]);
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
		assert(output.isUnique());
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
			output.add(leaf.intersection(shape));
		return output;
	}
	void queryRemove(CuboidSet& cuboids) const
	{
		Cuboid boundry = cuboids.boundry();
		for(const Cuboid& cuboid : queryGetAllCuboids(boundry))
			cuboids.remove(cuboid);
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
	[[nodiscard]] T get() const { return data; }
	[[nodiscard]] std::strong_ordering operator<=>(const RTreeDataWrapper<T, defaultValue>& other) const = default;
	[[nodiscard]] bool operator==(const RTreeDataWrapper<T, defaultValue>& other) const = default;
	[[nodiscard]] bool exists() const { return data != defaultValue; }
	[[nodiscard]] bool empty() const { return data == defaultValue; }
	[[nodiscard]] static RTreeDataWrapper<T, defaultValue> create(const T& p) { return {p}; }
	[[nodiscard]] static RTreeDataWrapper<T, defaultValue> null() { return {defaultValue}; }
};