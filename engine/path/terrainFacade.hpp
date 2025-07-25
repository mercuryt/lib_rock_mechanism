#pragma once

#include "terrainFacade.h"
// Needed for find path templates. Move to a hpp file?
#include "area/area.h"
#include "simulation/simulation.h"
#include "findPathInnerLoops.h"

template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathBreadthFirstWithoutMemo(const Point3D& start, const Facing4& startFacing, DestinationConditionT& destinationCondition, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getBreadthFirst();
	auto& memo = *pair.first;
	auto index = pair.second;
	auto output = PathInnerLoops::findPath<PathMemoBreadthFirst, anyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, moveType, start, startFacing, detour, adjacent, faction, maxRange);
	memo.reset();
	hasMemos.releaseBreadthFirst(index);
	return output;
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathDepthFirstWithoutMemo(const Point3D& from, const Facing4& startFacing, DestinationConditionT& destinationCondition, const Point3D& huristicDestination, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getDepthFirst();
	auto& memo = *pair.first;
	auto index = pair.second;
	memo.setDestination(huristicDestination);
	auto output = PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, moveType, from, startFacing, detour, adjacent, faction, maxRange);
	memo.reset();
	hasMemos.releaseDepthFirst(index);
	return output;
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionDepthFirst(DestinationConditionT& destinationCondition, PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& huristicDestination, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const
{
	memo.setDestination(huristicDestination);
	return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionDepthFirstWithoutMemo(DestinationConditionT& destinationCondition, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& huristicDestination, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const
{
	return findPathDepthFirstWithoutMemo<anyOccupiedPoint, decltype(destinationCondition)>(start, startFacing, destinationCondition, huristicDestination, shape, m_moveType, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionBreadthFirst(DestinationConditionT destinationCondition, PathMemoBreadthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const
{
	return PathInnerLoops::findPath<PathMemoBreadthFirst, anyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToConditionBreadthFirstWithoutMemo(DestinationConditionT& destinationCondition, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const
{
	return findPathBreadthFirstWithoutMemo<anyOccupiedPoint, decltype(destinationCondition)>(start, startFacing, destinationCondition, shape, m_moveType, detour, adjacent, faction, maxRange);
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
FindPathResult TerrainFacade::findPathToSpaceDesignationAndCondition(DestinationConditionT& destinationCondition, PathMemoBreadthFirst& memo, const SpaceDesignation designation, FactionId faction, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool adjacent, bool unreserved, const Distance& maxRange) const
{
	AreaHasSpaceDesignationsForFaction& hasSpaceDesignationsForFaction = m_area.m_spaceDesignations.getForFaction(faction);
	auto designationCondition = [&hasSpaceDesignationsForFaction, designation, destinationCondition](const Point3D& point, const Facing4& facing) -> std::pair<bool, Point3D>
	{
		if(!hasSpaceDesignationsForFaction.check(point, designation))
			return {false, point};
		return destinationCondition(point, facing);
	};
	if(!unreserved)
		// Clear faction to prevent checking destinaiton for reservation status.
		faction.clear();
	return findPathToConditionBreadthFirst<anyOccupiedPoint, decltype(designationCondition)>(designationCondition, memo, start, startFacing, shape, detour, adjacent, faction, maxRange);
}