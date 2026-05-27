#include "areaHasPaths.h"
#include "longRange.hpp"
template<bool adjacent, bool anyOccupied, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
PathResult AreaHasPathsForMoveType::pathToCondition(LongRangeConditionT& longRangeCondition, ShortRangeConditionT& shortRangeCondition, const PathParamaters& params) const
{
	if constexpr(adjacent)
		return longRangePath::getPathAdjacent(m_enterable, longRangeCondition, shortRangeCondition, params);
	else if constexpr(anyOccupied)
		return longRangePath::getPathAnyOccupied(m_enterable, longRangeCondition, shortRangeCondition, params);
	else
		return longRangePath::getPath(m_enterable, longRangeCondition, shortRangeCondition, params);
}
template<bool adjacent, bool anyOccupiedPoint, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
PathResult AreaHasPathsForMoveType::pathToConditionWithDesignation(SpaceDesignation designation, LongRangeConditionT& longRangeCondition, ShortRangeConditionT& shortRangeCondition, const PathParamaters& params) const
{
	const Space& space = params.area.getSpace();
	auto wrappedLongRangeConditon = [&](const Cuboid cuboid) -> bool
	{
		CuboidSet designated = space.designation_queryForFaction(cuboid, params.faction, designation);
		for(const Cuboid designatedCuboid : designated)
			if(longRangeCondition(designatedCuboid))
				return true;
		return false;
	};
	if constexpr(adjacent || anyOccupiedPoint)
	{
		auto wrappedShortRangeCondition = [&](const Cuboid cuboid) -> Point3D
		{
			CuboidSet designated = space.designation_queryForFaction(cuboid, params.faction, designation);
			for(const Cuboid designatedCuboid : designated)
			{
				Point3D result = shortRangeCondition(designatedCuboid);
				if(result.exists())
					return result;
			}
			return {};
		};
		return pathToCondition<adjacent, anyOccupiedPoint>(wrappedLongRangeConditon, wrappedShortRangeCondition, params);
	}
	else
	{
		auto wrappedShortRangeCondition = [&](const Point3D point, const Facing4 shortRangeFacing) -> Point3D
		{
			bool designated = space.designation_has(point, params.faction, designation);
			if(designated)
				return shortRangeCondition(point, shortRangeFacing);
			return {};
		};
		return pathToCondition<adjacent, anyOccupiedPoint>(wrappedLongRangeConditon, wrappedShortRangeCondition, params);
	}
}
template<bool adjacent, bool anyOccupied, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
Point3D AreaHasPathsForMoveType::accessableCondition(LongRangeConditionT& longRangeCondition, ShortRangeConditionT& shortRangeCondition, const PathParamaters& params) const
{
	assert(!params.returnPath);
	return pathToCondition<adjacent, anyOccupied>(longRangeCondition, shortRangeCondition, params).m_target;
}
