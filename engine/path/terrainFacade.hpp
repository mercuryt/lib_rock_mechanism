#pragma once

#include "terrainFacade.h"
// Needed for find path templates. Move to a hpp file?
#include "area/area.h"
#include "simulation/simulation.h"
#include "findPathInnerLoops.h"

template<typename DestinationConditionT, bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathBreadthFirstWithoutMemo(const Point3D& start, const Facing4& startFacing, DestinationConditionT& destinationCondition, const ShapeId& shape, const MoveTypeId& moveType, bool detour, const FactionId& faction, const Distance& maxRange) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getBreadthFirst();
	auto& memo = *pair.first;
	auto index = pair.second;
	FindPathResult output;
	if constexpr(adjacent)
		output = PathInnerLoops::findPathAdjacentTo<PathMemoBreadthFirst, DestinationConditionT, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, moveType, start, startFacing, detour, faction, maxRange);
	else
		output = PathInnerLoops::findPath<PathMemoBreadthFirst, anyOccupiedPoint, DestinationConditionT>(destinationCondition, m_area, *this, memo, shape, moveType, start, startFacing, detour, faction, maxRange);
	memo.reset();
	hasMemos.releaseBreadthFirst(index);
	return output;
}
template<typename DestinationConditionT, bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathDepthFirstWithoutMemo(const Point3D& from, const Facing4& startFacing, DestinationConditionT& destinationCondition, const Point3D& huristicDestination, const ShapeId& shape, const MoveTypeId& moveType, bool detour, const FactionId& faction, const Distance& maxRange) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getDepthFirst();
	auto& memo = *pair.first;
	auto index = pair.second;
	memo.setDestination(huristicDestination);
	FindPathResult output;
	if constexpr(adjacent)
		output = PathInnerLoops::findPathAdjacentTo<PathMemoDepthFirst, DestinationConditionT, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, moveType, from, startFacing, detour, faction, maxRange);
	else
		output = PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint, DestinationConditionT>(destinationCondition, m_area, *this, memo, shape, moveType, from, startFacing, detour, faction, maxRange);
	memo.reset();
	hasMemos.releaseDepthFirst(index);
	return output;
}
template<typename DestinationConditionT, bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToConditionDepthFirst(DestinationConditionT& destinationCondition, PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& huristicDestination, bool detour, const FactionId& faction, const Distance& maxRange) const
{
	memo.setDestination(huristicDestination);
	if constexpr(adjacent)
		return PathInnerLoops::findPathAdjacentTo<PathMemoDepthFirst, DestinationConditionT, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, maxRange);
	else
		return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint, DestinationConditionT>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, maxRange);
}
template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT, bool adjacent>
FindPathResult TerrainFacade::findPathToConditionDepthFirstWithoutMemo(DestinationConditionT& destinationCondition, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& huristicDestination, bool detour, const FactionId& faction, const Distance& maxRange) const
{
	return findPathDepthFirstWithoutMemo<decltype(destinationCondition), anyOccupiedPoint, adjacent>(start, startFacing, destinationCondition, huristicDestination, shape, m_moveType, detour, faction, maxRange);
}
template<typename DestinationConditionT, bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToConditionBreadthFirst(DestinationConditionT destinationCondition, PathMemoBreadthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, const FactionId& faction, const Distance& maxRange) const
{
	if constexpr(adjacent)
	{
		static_assert(!anyOccupiedPoint);
		return PathInnerLoops::findPathAdjacentTo<PathMemoBreadthFirst, DestinationConditionT, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, maxRange);
	}
	else
		return PathInnerLoops::findPath<PathMemoBreadthFirst, anyOccupiedPoint, DestinationConditionT>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, maxRange);
}
template<typename DestinationConditionT, bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToConditionBreadthFirstWithoutMemo(DestinationConditionT& destinationCondition, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, const FactionId& faction, const Distance& maxRange) const
{
	return findPathBreadthFirstWithoutMemo<DestinationConditionT, anyOccupiedPoint, adjacent>(start, startFacing, destinationCondition, shape, m_moveType, detour, faction, maxRange);
}
template<CuboidToBool DestinationConditionT, bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToSpaceDesignationAndCondition(DestinationConditionT& destinationCondition, PathMemoBreadthFirst& memo, const SpaceDesignation designation, FactionId faction, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool unreserved, const Distance& maxRange) const
{
	const AreaHasSpaceDesignationsForFaction& hasSpaceDesignationsForFaction = m_area.m_spaceDesignations.getForFaction(faction);
	if constexpr(adjacent || anyOccupiedPoint)
	{
		const auto designationCondition = [&](const Cuboid& cuboid) -> std::pair<bool, Point3D>
		{
			const Point3D point = hasSpaceDesignationsForFaction.queryPointWithCondition(cuboid, designation, destinationCondition);
			return {point.exists(), point};
		};
		return findPathToConditionBreadthFirst<decltype(designationCondition), anyOccupiedPoint, adjacent>(designationCondition, memo, start, startFacing, shape, detour, faction, maxRange);
	}
	else
	{
		if(!unreserved)
			// Clear faction to prevent checking destinaiton for reservation status.
			faction.clear();
		if constexpr(DestinationCondition<DestinationConditionT>)
			return findPathToConditionBreadthFirst<DestinationConditionT, false, false>(destinationCondition, memo, start, startFacing, shape, detour, faction, maxRange);
		else
		{
			auto designationCondition = [&](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D>
			{
				const Point3D result = hasSpaceDesignationsForFaction.queryPointWithCondition(point, designation, destinationCondition);
				return {result.exists(), result};
			};
			return findPathToConditionBreadthFirst<decltype(designationCondition), false, false>(designationCondition, memo, start, startFacing, shape, detour, faction, maxRange);
		}
	}
}