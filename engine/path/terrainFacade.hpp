#pragma once

#include "terrainFacade.h"
// Needed for find path templates. Move to a hpp file?
#include "area/area.h"
#include "simulation/simulation.h"

template<bool anyOccupiedPoint, bool adjacent, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
FindPathResult TerrainFacade::findPathBreadthFirstWithoutMemo(const Point3D start, const Facing4 startFacing, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const ShapeId shape, bool detour, const FactionId faction, const Distance maxRange) const
{
	longRangePath::LongRangeMemo& memo = longRangePath::checkOutMemo();
	FindPathResult output = findPathToConditionBreadthFirst<anyOccupiedPoint, adjacent>(longRangeCondition, shortRangeCondition, memo, start, startFacing, shape, detour, faction, maxRange);
	longRangePath::returnMemo(memo);
	return output;
}
template<bool anyOccupiedPoint, bool adjacent, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
FindPathResult TerrainFacade::findPathDepthFirstWithoutMemo(const Point3D start, const Facing4 startFacing, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const Point3D huristicDestination, const ShapeId shape, bool detour, const FactionId faction, const Distance maxRange) const
{
	longRangePath::LongRangeMemo& memo = longRangePath::checkOutMemo();
	FindPathResult output = findPathToConditionDepthFirst(longRangeCondition, shortRangeCondition, memo, start, startFacing, shape, huristicDestination, detour, faction, maxRange);
	longRangePath::returnMemo(memo);
	return output;
}
template<bool anyOccupiedPoint, bool adjacent, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
FindPathResult TerrainFacade::findPathToConditionDepthFirst(LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, longRangePath::LongRangeMemo& memo, const Point3D start, const Facing4 startFacing, const ShapeId shape, const Point3D huristicDestination, bool detour, const FactionId faction, const Distance maxRange) const
{
	bool singleTile = !Shape::getIsMultiTile(shape);
	const CuboidSet& occupied = Shape::getCuboidsOccupiedAt(shape, m_area.getSpace(), start, startFacing);
	auto [path, target] = longRangePath::getPath(m_area, m_enterable, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, faction, true, singleTile, detour, adjacent, anyOccupiedPoint, huristicDestination);
	return {path, target, target == start};
}
template<bool anyOccupiedPoint, bool adjacent, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
FindPathResult TerrainFacade::findPathToConditionBreadthFirst(LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, longRangePath::LongRangeMemo& memo, const Point3D start, const Facing4 startFacing, const ShapeId shape, bool detour, const FactionId faction, const Distance maxRange) const
{
	bool singleTile = !Shape::getIsMultiTile(shape);
	const CuboidSet& occupied = Shape::getCuboidsOccupiedAt(shape, m_area.getSpace(), start, startFacing);
	auto [path, target] = longRangePath::getPath(m_area, m_enterable, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, faction, false, singleTile, detour, adjacent, anyOccupiedPoint, maxRange);
	return {path, target, target == start};
}
// TODO: this method is pointless.
template<bool anyOccupiedPoint, bool adjacent, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
FindPathResult TerrainFacade::findPathToConditionBreadthFirstWithoutMemo(LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const Point3D start, const Facing4 startFacing, const ShapeId shape, bool detour, const FactionId faction, const Distance maxRange) const
{
	return findPathBreadthFirstWithoutMemo<anyOccupiedPoint, adjacent>(start, startFacing, longRangeCondition, shortRangeCondition, shape, detour, faction, maxRange);
}
template<bool anyOccupiedPoint, bool adjacent, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
FindPathResult TerrainFacade::findPathToSpaceDesignationAndCondition(LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, longRangePath::LongRangeMemo& memo, const SpaceDesignation designation, FactionId faction, const Point3D start, const Facing4 startFacing, const ShapeId shape, bool detour, bool unreserved, const Distance maxRange) const
{
	if(!unreserved)
		faction.clear();
	const AreaHasSpaceDesignationsForFaction& hasSpaceDesignationsForFaction = m_area.m_spaceDesignations.getForFaction(faction);
	auto wrappedLongRangeCondition = [&](const Cuboid cuboid ) -> bool
	{
		// TODO:(optimization) This could be better.
		return hasSpaceDesignationsForFaction.check(cuboid, designation) && longRangeCondition(cuboid);
	};
	if constexpr(adjacent || anyOccupiedPoint)
	{
		auto wrappedShortRangeCondition = [&](const Cuboid cuboid) -> Point3D
		{
			return hasSpaceDesignationsForFaction.queryPointWithCondition(cuboid, designation, shortRangeCondition);
		};
		return findPathToConditionBreadthFirst<anyOccupiedPoint, adjacent>(longRangeCondition, shortRangeCondition, memo, start, startFacing, shape, detour, faction, maxRange);
	}
	else
	{
		auto wrappedShortRangeCondition = [&](const Point3D location, const Facing4) -> Point3D
		{
			return hasSpaceDesignationsForFaction.queryPointWithCondition(location, designation, shortRangeCondition);
		};
		return findPathToConditionBreadthFirst<anyOccupiedPoint, adjacent>(longRangeCondition, shortRangeCondition, memo, start, startFacing, shape, detour, faction, maxRange);
	}
}