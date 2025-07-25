#include "longRange.h"
#include "../space/space.h"
#include "../area/area.h"
#include "../config.h"
#include "../partitionNotify.h"
void LongRangePathFinderForMoveType::merge(const LongRangePathNodeIndex& mergeInto, const LongRangePathNodeIndex& mergeFrom)
{
	assert(mergeInto != mergeFrom);
	auto& from = m_data[mergeFrom];
	auto& into = m_data[mergeInto];
	assert(!from.destroy);
	assert(!into.destroy);
	into.contents.addSet(from.contents);
	for(const Point3D& point : from.contents.getView())
	{
		m_pathNodesByPoint.maybeRemove(point);
		m_pathNodesByPoint.maybeInsert(point, mergeInto);
	}
	into.adjacent.maybeInsertAll(from.adjacent);
	from.destroy = true;
}
LongRangePathNodeIndex LongRangePathFinderForMoveType::create(const Point3D& point)
{
	Cuboid cuboid(point, point);
	m_data.emplaceBack(cuboid);
	auto index = LongRangePathNodeIndex::create(m_data.size() - 1);
	for(const auto& point : cuboid)
	{
		m_pathNodesByPoint.maybeRemove(point);
		m_pathNodesByPoint.maybeInsert(point, index);
	}
	return index;
}
void LongRangePathFinderForMoveType::addPointToNode(const LongRangePathNodeIndex& node, const Point3D& point)
{
	assert(!m_data[node].destroy);
	m_data[node].contents.addAndExtend(point);
	m_pathNodesByPoint.maybeRemove(point);
	m_pathNodesByPoint.maybeInsert(point, node);
}
void LongRangePathFinderForMoveType::setPointPathable(Area& area, const Point3D& point)
{
	std::vector<LongRangePathNodeIndex> adjacentNodeIndices;
	Space& space = area.getSpace();
	LongRangePathNodeIndex createdOrAddedToIndex;
	for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
	{
		const auto& nodeIndex = m_pathNodesByPoint.queryGetOne(adjacent);
		if(
			nodeIndex.exists() &&
			!std::ranges::contains(adjacentNodeIndices, nodeIndex) &&
			canAbsorbPoint(m_data[nodeIndex], point)
		)
		{
			assert(!m_data[nodeIndex].destroy);
			adjacentNodeIndices.push_back(nodeIndex);
		}
	}
	if(adjacentNodeIndices.empty())
		createdOrAddedToIndex = create(point);
	else
	{
		// Find largest of adjacentNodeIndices.
		auto iter = std::ranges::max_element(adjacentNodeIndices, {}, [&](const LongRangePathNodeIndex& index) -> int {
			return m_data[index].contents.size();
		});
		const auto& largest = *iter;
		// Merge with largest.
		addPointToNode(largest, point);
		// Attempt to merge largest with remainder.
		for(const auto& index : adjacentNodeIndices)
			if(index != largest)
			{
				if(canAbsorbNode(m_data[largest], m_data[index]))
					merge(largest, index);
				else
					m_data[largest].adjacent.maybeInsert(index);
			}
		createdOrAddedToIndex = largest;
	}
	clearDestroyed();
	auto& node = m_data[createdOrAddedToIndex];
	for(auto& [faction, designation] : area.m_spaceDesignations.getForPoint(point))
		++node.designations.getOrCreate(faction).getOrInsert(designation, Quantity::create(0));
}
void LongRangePathFinderForMoveType::setPointNotPathable(Area& area, const Point3D& point)
{
	auto& nodeIndex = m_pathNodesByPoint.queryGetOne(point);
	assert(nodeIndex.exists());
	auto& node = m_data[nodeIndex];
	Space& space = area.getSpace();
	SmallSet<LongRangePathNodeIndex> potentiallyNoLongerAdjacentNodes;
	for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
	{
		const auto& adjacentIndex = m_pathNodesByPoint.queryGetOne(adjacent);
		if(adjacentIndex != nodeIndex)
			potentiallyNoLongerAdjacentNodes.maybeInsert(adjacentIndex);
	}
	SmallSet<Cuboid> toCreate = node.contents.removeAndReturnNoLongerAdjacentCuboids(point);
	for(const auto& adjacentIndex : potentiallyNoLongerAdjacentNodes)
	{
		auto& adjacentNode = m_data[adjacentIndex];
		if(!adjacentNode.contents.isAdjacent(node.contents))
		{
			node.adjacent.erase(adjacentIndex);
			adjacentNode.adjacent.erase(nodeIndex);
		}
	}
	if(node.contents.empty())
	{
		assert(toCreate.empty());
		node.destroy = true;
	}
	else
	{
		for(auto& [faction, designation] : area.m_spaceDesignations.getForPoint(point))
		{
			auto& forFaction = node.designations[faction];
			if(forFaction[designation] == 1)
			{
				if(forFaction.size() == 1)
					node.designations.erase(faction);
				else
					forFaction.erase(designation);
			}
			else
				--forFaction[designation];
		}
	}
	clearDestroyed();
	for(const auto& cuboid : toCreate)
		create(cuboid);
}
void LongRangePathFinderForMoveType::clearDestroyed()
{
	auto condition = [&](const LongRangePathNodeIndex& node) { return m_data[node].destroy; };
	auto callback = [&](const LongRangePathNodeIndex& oldIndex, const LongRangePathNodeIndex& newIndex) {
		LongRangePathNode& oldNode = m_data[oldIndex];
		for(const auto& adjacentIndex : oldNode.adjacent)
			m_data[adjacentIndex].adjacent.update(oldIndex, newIndex);
		for(const auto& point : oldNode.contents.getView())
		{
			m_pathNodesByPoint.maybeRemove(point);
			m_pathNodesByPoint.maybeInsert(point, newIndex);
		}
	};
	removeNotify(m_data, condition, callback);
}
bool LongRangePathFinderForMoveType::canAbsorbNode(const LongRangePathNode& a, LongRangePathNode& b)
{
	assert(!b.destroy);
	return canAbsorbCuboid(a, b.contents.getBoundingBox());
}
bool LongRangePathFinderForMoveType::canAbsorbPoint(const LongRangePathNode& node, const Point3D& point)
{
	return canAbsorbCuboid(node, {point, point});
}
bool LongRangePathFinderForMoveType::canAbsorbCuboid(const LongRangePathNode& node, const Cuboid& cuboid)
{
	assert(!node.destroy);
	const Cuboid sum = node.contents.getBoundingBox().sum(cuboid);
	return (
		sum.dimensionForFacing(Facing6::Below) <= Config::maxSizeOfLongRangePathNode &&
		sum.dimensionForFacing(Facing6::South) <= Config::maxSizeOfLongRangePathNode &&
		sum.dimensionForFacing(Facing6::East) <= Config::maxSizeOfLongRangePathNode
	);
}
SmallSet<LongRangePathNodeIndex> LongRangePathFinderForMoveType::pathToPoint(const Point3D& start, const Point3D& destination)
{
	const LongRangePathNodeIndex& index = m_pathNodesByPoint.queryGetOne(destination);
	auto condition = [&](const LongRangePathNodeIndex& otherIndex) { return index == otherIndex; };
	return pathToCondition(start, condition);
}
SmallSet<LongRangePathNodeIndex> LongRangePathFinderForMoveType::pathToDesignation(const Point3D& start, const SpaceDesignation& designation, const FactionId& faction)
{
	auto condition = [&](const LongRangePathNodeIndex& otherIndex) -> bool {
		auto designations = m_data[otherIndex].designations;
		auto found = designations.find(faction);
		if(found == designations.end())
			return false;
		return found->second.contains(designation);
	};
	return pathToCondition(start, condition);
}