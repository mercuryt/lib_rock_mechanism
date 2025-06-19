#pragma once
#include "../geometry/cuboidSet.h"
#include "../dataStructures/smallSet.h"
#include "../dataStructures/smallMap.h"
#include "../types.h"
#include "../dataStructures/strongVector.h"
#include "../callbackTypes.h"
#include "../designations.h"
struct LongRangePathNode
{
	CuboidSetWithBoundingBoxAdjacent contents;
	SmallSet<LongRangePathNodeIndex> adjacent;
	SmallMap<FactionId, SmallMap<BlockDesignation, Quantity>> designations;
	bool destroy = false;
	LongRangePathNode() = default;
	LongRangePathNode(const Cuboid& cuboid) { contents.addAndExtend(cuboid); }
	LongRangePathNode(const LongRangePathNode& other) : contents(other.contents), adjacent(other.adjacent), designations(other.designations), destroy(other.destroy) { }
	LongRangePathNode& operator=(const LongRangePathNode& other)
	{
		contents = other.contents;
		adjacent = other.adjacent;
		designations = other.designations;
		destroy = other.destroy;
		return *this;
	}
};
class LongRangePathFinderForMoveType
{
	StrongVector<LongRangePathNode, LongRangePathNodeIndex> m_data;
	StrongVector<LongRangePathNodeIndex, BlockIndex> m_pathNodesByBlock;
	void merge(Blocks& blocks, const LongRangePathNodeIndex& mergeInto, const LongRangePathNodeIndex& mergeFrom);
	LongRangePathNodeIndex create(Blocks& blocks, const BlockIndex& block);
	LongRangePathNodeIndex create(Blocks& blocks, const Cuboid& cuboid);
	void addBlockToNode(Blocks& blocks, const LongRangePathNodeIndex& node, const BlockIndex& block);
public:
	void setBlockPathable(Area& area, const BlockIndex& block);
	void setBlockNotPathable(Area& area, const BlockIndex& block);
	void clearDestroyed(Blocks& blocks);
	void recordBlockHasDesignation(Area& area, const BlockIndex& block, const BlockDesignation& designation);
	void recordBlockDoesNotHaveDesignation(Area& area, const BlockIndex& block, const BlockDesignation& designation);
	[[nodiscard]] bool canAbsorbNode(const LongRangePathNode& a, LongRangePathNode& b);
	[[nodiscard]] bool canAbsorbBlock(const Blocks& blocks, const LongRangePathNode& node, const BlockIndex& block);
	[[nodiscard]] bool canAbsorbCuboid(const LongRangePathNode& node, const Cuboid& cuboid);
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToBlock(const BlockIndex& start, const BlockIndex& destination);
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToDesignation(const BlockIndex& start, const BlockDesignation& designation, const FactionId& faction);
	template<DestinationCondition Condition>
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToBlockCondition(const Blocks& blocks, const BlockIndex& start, Condition&& condition)
	{
		auto nodeCondition = [&](const LongRangePathNode& node) -> bool {
			for(const BlockIndex& block : node.contents.getView(blocks))
				// Ignore facing.
				if(condition(block, Facing4::North))
					return true;
			return false;
		};
		return pathToCondition(start, nodeCondition);
	}
	template<typename Condition>
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToCondition(const BlockIndex& start, Condition&& condition)
	{
		SmallMap<LongRangePathNodeIndex, LongRangePathNodeIndex> closed;
		std::deque<LongRangePathNodeIndex> open;
		const LongRangePathNodeIndex startNodeIndex = m_pathNodesByBlock[start];
		closed.insert(startNodeIndex, LongRangePathNodeIndex::max());
		open.push_back(startNodeIndex);
		while(!open.empty())
		{
			LongRangePathNodeIndex current = open.front();
			open.pop_front();
			if(condition(current))
			{
				SmallSet<LongRangePathNodeIndex> output;
				while(true)
				{
					output.insert(current);
					const auto& previous = closed[current];
					if(previous == LongRangePathNodeIndex::max())
						return output;
					current = previous;
				}
			}
			for(const LongRangePathNodeIndex& adjacentIndex : m_data[current].adjacent)
				if(!closed.contains(adjacentIndex))
				{
					closed.insert(adjacentIndex, current);
					open.push_back(adjacentIndex);
				}
		}
		// No path found.
		return {};
	}
	// TODO: path to itemQuery, path to fluid.
};
class AreaHasLongRangePathFinder
{
	SmallMap<MoveTypeId, LongRangePathFinderForMoveType> m_pathFinders;
	LongRangePathNodeIndex m_nextNodeIndex = LongRangePathNodeIndex::create(0);
public:
	void registerMoveType(const MoveTypeId& moveType);
	LongRangePathFinderForMoveType get(const MoveTypeId& moveType);
};