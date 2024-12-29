#pragma once

#include "terrainFacade.h"
// Needed for find path templates. Move to a hpp file?
#include "area.h"
#include "simulation.h"
#include "findPathInnerLoops.h"

template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathBreadthFirstWithoutMemo(const BlockIndex& start, const Facing& startFacing, DestinationConditionT& destinationCondition, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getBreadthFirst(m_area);
	auto& memo = *pair.first;
	auto index = pair.second;
	auto output = PathInnerLoops::findPath<PathMemoBreadthFirst, anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, moveType, start, startFacing, detour, adjacent, faction, maxRange);
	memo.reset();
	hasMemos.releaseBreadthFirst(index);
	return output;
}
template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathDepthFirstWithoutMemo(const BlockIndex& from, const Facing& startFacing, DestinationConditionT& destinationCondition, const BlockIndex& huristicDestination, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getDepthFirst(m_area);
	auto& memo = *pair.first;
	auto index = pair.second;
	memo.setDestination(huristicDestination);
	auto output = PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, moveType, from, startFacing, detour, adjacent, faction, maxRange);
	memo.reset();
	hasMemos.releaseDepthFirst(index);
	return output;
}
template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionDepthFirst(DestinationConditionT& destinationCondition, PathMemoDepthFirst& memo, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& huristicDestination, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const
{
	memo.setDestination(huristicDestination);
	return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionDepthFirstWithoutMemo(DestinationConditionT& destinationCondition, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& huristicDestination, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const
{
	return findPathDepthFirstWithoutMemo<anyOccupiedBlock, decltype(destinationCondition)>(start, startFacing, destinationCondition, huristicDestination, shape, m_moveType, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionBreadthFirst(DestinationConditionT destinationCondition, PathMemoBreadthFirst& memo, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const
{
	return PathInnerLoops::findPath<PathMemoBreadthFirst, anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionBreadthFirstWithoutMemo(DestinationConditionT& destinationCondition, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const
{
	return findPathBreadthFirstWithoutMemo<anyOccupiedBlock, decltype(destinationCondition)>(start, startFacing, destinationCondition, shape, m_moveType, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToBlockDesignationAndCondition(DestinationConditionT& destinationCondition, PathMemoBreadthFirst& memo, const BlockDesignation designation, FactionId faction, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, bool detour, bool adjacent, bool unreserved, const DistanceInBlocks& maxRange) const
{
	AreaHasBlockDesignationsForFaction& hasBlockDesginationsForFaction = m_area.m_blockDesignations.getForFaction(faction);
	auto designationCondition = [&hasBlockDesginationsForFaction, designation, destinationCondition](const BlockIndex& block, const Facing& facing) -> std::pair<bool, BlockIndex>
	{
		if(!hasBlockDesginationsForFaction.check(block, designation))
			return {false, block};
		return destinationCondition(block, facing);
	};
	if(!unreserved)
		// Clear faction to prevent checking destinaiton for reservation status.
		faction.clear();
	return findPathToConditionBreadthFirst<anyOccupiedBlock, decltype(designationCondition)>(designationCondition, memo, start, startFacing, shape, detour, adjacent, faction, maxRange);
}