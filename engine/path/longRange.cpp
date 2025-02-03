#include "longRange.h"
#include "../blocks/blocks.h"
#include "../area.h"
#include "../config.h"
#include "../partitionNotify.h"
void LongRangePathFinderForMoveType::merge(Blocks& blocks, const LongRangePathNodeIndex& mergeInto, const LongRangePathNodeIndex& mergeFrom)
{
	assert(mergeInto != mergeFrom);
	auto& from = m_data[mergeFrom];
	auto& into = m_data[mergeInto];
	assert(!from.destroy);
	assert(!into.destroy);
	into.contents.addSet(from.contents);
	for(const BlockIndex& block : from.contents.getView(blocks))
		m_pathNodesByBlock[block] = mergeInto;
	into.adjacent.maybeInsertAll(from.adjacent);
	from.destroy = true;
}
LongRangePathNodeIndex LongRangePathFinderForMoveType::create(Blocks& blocks, const BlockIndex& block)
{
	Cuboid cuboid(blocks, block, block);
	m_data.emplaceBack(cuboid);
	auto index = LongRangePathNodeIndex::create(m_data.size() - 1);
	for(const auto& block : cuboid.getView(blocks))
		m_pathNodesByBlock[block] = index;
	return index;
}
void LongRangePathFinderForMoveType::addBlockToNode(Blocks& blocks, const LongRangePathNodeIndex& node, const BlockIndex& block)
{
	assert(!m_data[node].destroy);
	m_data[node].contents.addAndExtend(blocks, block);
	m_pathNodesByBlock[block] = node;
}
void LongRangePathFinderForMoveType::setBlockPathable(Area& area, const BlockIndex& block)
{
	std::vector<LongRangePathNodeIndex> adjacentNodeIndices;
	Blocks& blocks = area.getBlocks();
	LongRangePathNodeIndex createdOrAddedToIndex;
	for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(block))
	{
		const auto& nodeIndex = m_pathNodesByBlock[adjacent];
		if(
			nodeIndex.exists() &&
			!std::ranges::contains(adjacentNodeIndices, nodeIndex) &&
			canAbsorbBlock(blocks, m_data[nodeIndex], block)
		)
		{
			assert(!m_data[nodeIndex].destroy);
			adjacentNodeIndices.push_back(nodeIndex);
		}
	}
	if(adjacentNodeIndices.empty())
		createdOrAddedToIndex = create(blocks, block);
	else
	{
		// Find largest of adjacentNodeIndices.
		auto iter = std::ranges::max_element(adjacentNodeIndices, {}, [&](const LongRangePathNodeIndex& index) -> int {
			return m_data[index].contents.size();
		});
		const auto& largest = *iter;
		// Merge with largest.
		addBlockToNode(blocks, largest, block);
		// Attempt to merge largest with remainder.
		for(const auto& index : adjacentNodeIndices)
			if(index != largest)
			{
				if(canAbsorbNode(m_data[largest], m_data[index]))
					merge(blocks, largest, index);
				else
					m_data[largest].adjacent.maybeInsert(index);
			}
		createdOrAddedToIndex = largest;
	}
	clearDestroyed(blocks);
	auto& node = m_data[createdOrAddedToIndex];
	for(auto& [faction, designation] : area.m_blockDesignations.getForBlock(block))
		++node.designations.getOrCreate(faction).getOrInsert(designation, Quantity::create(0));
}
void LongRangePathFinderForMoveType::setBlockNotPathable(Area& area, const BlockIndex& block)
{
	auto& nodeIndex = m_pathNodesByBlock[block];
	assert(nodeIndex.exists());
	auto& node = m_data[nodeIndex];
	Blocks& blocks = area.getBlocks();
	SmallSet<LongRangePathNodeIndex> potentiallyNoLongerAdjacentNodes;
	for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(block))
	{
		const auto& adjacentIndex = m_pathNodesByBlock[adjacent];
		if(adjacentIndex != nodeIndex)
			potentiallyNoLongerAdjacentNodes.maybeInsert(adjacentIndex);
	}
	SmallSet<Cuboid> toCreate = node.contents.removeAndReturnNoLongerAdjacentCuboids(blocks, block);
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
		for(auto& [faction, designation] : area.m_blockDesignations.getForBlock(block))
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
	clearDestroyed(blocks);
	for(const auto& cuboid : toCreate)
		create(blocks, cuboid);
}
void LongRangePathFinderForMoveType::clearDestroyed(Blocks& blocks)
{
	auto condition = [&](const LongRangePathNodeIndex& node) { return m_data[node].destroy; };
	auto callback = [&](const LongRangePathNodeIndex& oldIndex, const LongRangePathNodeIndex& newIndex) {
		LongRangePathNode& oldNode = m_data[oldIndex];
		for(const auto& adjacentIndex : oldNode.adjacent)
			m_data[adjacentIndex].adjacent.update(oldIndex, newIndex);
		for(const auto& block : oldNode.contents.getView(blocks))
			m_pathNodesByBlock[block] = newIndex;
	};
	removeNotify(m_data, condition, callback);
}
bool LongRangePathFinderForMoveType::canAbsorbNode(const LongRangePathNode& a, LongRangePathNode& b)
{
	assert(!b.destroy);
	return canAbsorbCuboid(a, b.contents.getBoundingBox());
}
bool LongRangePathFinderForMoveType::canAbsorbBlock(const Blocks& blocks, const LongRangePathNode& node, const BlockIndex& block)
{
	auto point = blocks.getCoordinates(block);
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
SmallSet<LongRangePathNodeIndex> LongRangePathFinderForMoveType::pathToBlock(const BlockIndex& start, const BlockIndex& destination)
{
	const LongRangePathNodeIndex& index = m_pathNodesByBlock[destination];
	auto condition = [&](const LongRangePathNodeIndex& otherIndex) { return index == otherIndex; };
	return pathToCondition(start, condition);
}
SmallSet<LongRangePathNodeIndex> LongRangePathFinderForMoveType::pathToDesignation(const BlockIndex& start, const BlockDesignation& designation, const FactionId& faction)
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