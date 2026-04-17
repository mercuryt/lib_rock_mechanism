#pragma once
#include "longRange.h"
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/linePath.h"
#include "../geometry/rectangle.h"
#include "../space/space.h"
#include "../area/area.h"
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT, bool depthFirst, bool singleTile, bool detour>
std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPathInnerLoop(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const Point3D huristicDestination)
{
	// TODO: if cuboid is narrower then width in x or y prove a path for shape from previous cuboid to next cuboid before adding to openList.
	if constexpr(depthFirst)
		assert(huristicDestination.exists());
	if constexpr(detour)
		static_assert(depthFirst);
	// Check if short range condition is true for starting position and facing.
	Point3D targetFoundForStartingPosition = shortRangeCondition(start, startFacing);
	if(targetFoundForStartingPosition.exists())
		return {{}, targetFoundForStartingPosition};
	const Distance width = Shape::getWidth(shape);
	const Distance length = Shape::getLength(shape);
	const Distance height = Shape::getHeight(shape);
	std::vector<OpenListData>& openList = memo.openList;
	openList.clear();
	const Cuboid startCuboid = rtree.queryGetOneCuboid(start);
	bool enableShortRangeForStart = checkCuboidNotFreelyNavigable(startCuboid, width, length, height);
	openList.emplace_back(startCuboid, Cuboid::null(), 0, enableShortRangeForStart);
	SmallMap<Cuboid, SmallMap<Cuboid, int>>& closedListToFromWithAccumulatedCost = memo.closedListToFromWithAccumulatedCost;
	closedListToFromWithAccumulatedCost.clear();
	// StartCuboid has no 'from'.
	closedListToFromWithAccumulatedCost.emplace(startCuboid, SmallMap<Cuboid, int>());
	while(!openList.empty())
	{
		std::ranges::sort(openList, std::greater{}, &OpenListData::score);
		const Cuboid previous = openList.back().previous;
		const Cuboid current = openList.back().current;
		const int cost = openList.back().cost;
		const bool shortRangeEnabled = openList.back().shortRange;
		openList.pop_back();
		if(shortRangeEnabled)
		{
			for(const Cuboid accessableCuboid : longRangePath::getAccessableCuboids<detour>(area, rtree, current, previous, shape, occupied, memo.shortRangeMemo))
			{
				const auto [cuboids, destination, pointBeforeDestination, target] = withEachCuboid<depthFirst, singleTile>(area, rtree, accessableCuboid, current, startCuboid, start, shape, width, length, height, occupied, longRangeCondition, shortRangeCondition, cost, memo, huristicDestination);
				if(destination.exists())
				{
					// TODO: Single tile optimization uses string pulling.
					SmallSet<Point3D> path = cuboidPathToPointPath<detour>(area, shape, cuboids, start, destination, pointBeforeDestination, memo.shortRangeMemo);
					return {path, target};
				}
			}
		}
		else
		{
			SmallSet<Point3D> output;
			Point3D target;
			rtree.queryForEachCuboids(current.inflated({1}), [&](const Cuboid adjacentCuboid){
				if(!output.empty())
					// Path has been found already.
					// TODO: signal rtree to abort query early.
					return;
				const auto [cuboids, destination, pointBeforeDestination, _target] = withEachCuboid<depthFirst, singleTile>(area, rtree, adjacentCuboid, current, startCuboid, start, shape, width, length, height, occupied, longRangeCondition, shortRangeCondition, cost, memo, huristicDestination);
				if(destination.exists())
				{
					assert(!detour);
					output = cuboidPathToPointPath(area, shape, cuboids, start, destination, pointBeforeDestination, memo.shortRangeMemo);
					target = _target;
				}
			});
			if(!output.empty())
				return {output, target};
		}
	}
	// No route found.
	return {{}, Point3D::null()};
}
// A helper for getPath.
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT, bool depthFirst, bool singleTile, bool detour>
[[nodiscard]] std::tuple<SmallSet<Cuboid>, Point3D, Point3D, Point3D> longRangePath::withEachCuboid(const Area& area, const auto& rtree, const Cuboid cuboid, const Cuboid previous, const Cuboid startCuboid, const Point3D start, const ShapeId shape, const int width, const int length, const int height, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const int cost, LongRangeMemo& memo, const Point3D huristicDestination)
{
	const Space& space = area.getSpace();
	bool shortRangeEnabled = false;
	std::vector<OpenListData>& openList = memo.openList;
	SmallMap<Cuboid, SmallMap<Cuboid, int>>& closedListToFromWithAccumulatedCost = memo.closedListToFromWithAccumulatedCost;
	if constexpr(!singleTile)
	{
		if(!checkPortalSize(cuboid, previous, width, length, height))
		{
			// Shape does not fit through gap between cuboids but might still be able to pass due to a hypothetical third cuboid.
			// Find a point in current which is adjacent to cuboid where shape can fit facing cuboid and can step forward.
			Cuboid from = cuboid.inflated({1}).intersection(previous);
			bool found = false;
			for(const Point3D point : from)
				if(space.shape_canFitEverWithAnyFacing(point, shape))
				{
					Cuboid adjacent = point.getAdjacent();
					for(const Point3D adjacentPoint : adjacent.intersection(cuboid))
					{
						// check possible destinations.
						if(space.shape_canFitEverWithAnyFacing(adjacentPoint, shape))
						{
							found = true;
							break;
						}
					}
					if(found)
						break;
				}
			if(!found)
				// No way to pass through this portal, continue to the next open list candidate.
				return {{}, Point3D::null(), Point3D::null(), Point3D::null()};
			shortRangeEnabled = true;
		}
		if(!shortRangeEnabled)
			shortRangeEnabled = checkCuboidNotFreelyNavigable(cuboid, width, length, height);
	}
	if constexpr(detour)
		// If detouring then enable short range mode for any cuboid which contains dynamic shapes.
		if(!shortRangeEnabled && space.shape_queryAnyDynamic(cuboid))
				shortRangeEnabled = true;
	if(shortRangeEnabled)
	{
		// If short range is enabled for cuboid then we must check for both 'to' and 'from' in closedList.
		auto found = closedListToFromWithAccumulatedCost.find(cuboid);
		if(found != closedListToFromWithAccumulatedCost.end() && found->contains(previous))
			return {{}, Point3D::null(), Point3D::null(), Point3D::null()};
	}
	// If short range is disabled we only need to check 'to'. 'From' doesn't matter becuase every point in cuboid is accessable.
	else if(closedListToFromWithAccumulatedCost.contains(cuboid))
		return {{}, Point3D::null(), Point3D::null(), Point3D::null()};
	if(longRangeCondition(cuboid))
	{
		// There is a potential destination reachable from some point(s) in this cuboid.
		auto [destination, pointBeforeDestination, target] = getAccessableDestination<detour>(area, rtree, cuboid, previous, shape, shortRangeCondition,memo.shortRangeMemo, occupied);
		if(destination.exists())
		{
			// A destination is accessable, backtrack through closedList to collect cuboids.
			SmallSet<Cuboid> path;
			Cuboid currentHistory = cuboid;
			while(currentHistory != startCuboid)
			{
				path.insert(currentHistory);
				// Pick the from-entry with the lowest cost
				const SmallMap<Cuboid, int>& fromMap = closedListToFromWithAccumulatedCost[currentHistory];
				currentHistory = std::ranges::min_element(fromMap, {}, [](const auto& pair){ return pair.second; })->first;
			}
			return {path, destination, pointBeforeDestination, target};
		}
	}
	// No destination found here, add to open list.
	int newCost = cost;
	if(previous.exists())
		newCost += previous.distanceTo(cuboid);
	else
		newCost += start.distanceTo(cuboid);
	int score = newCost;
	if constexpr(depthFirst)
		score += cuboid.center().distanceTo(huristicDestination);
	closedListToFromWithAccumulatedCost.getOrCreate(cuboid).emplace(previous, newCost);
	openList.emplace_back(cuboid, previous, newCost, score, shortRangeEnabled);
	return {{}, Point3D::null(), Point3D::null(), Point3D::null()};
}
// Find which cuboids adjacent to current can be entered. Short range pathing to be used when Cuboid is not freely navigable or shape enters through a constrained portal.
template<bool detour>
[[nodiscard]] SmallSet<Cuboid> longRangePath::getAccessableCuboids(const Area& area, const auto& rtree, const Cuboid current, const Cuboid previous, const ShapeId shape, const CuboidSet& occupied, ShortRangeMemo& memo)
{
	Cuboid inflatedCurrent = current.inflated({1});
	SmallSet<Cuboid> candidates = rtree.queryGetAllLeaves(inflatedCurrent);
	SmallSet<Cuboid> output;
	candidates.erase(current);
	SmallSet<Point3D>& openList = memo.openList;
	openList.clear();
	openList.reserve(current.volume() / 2);
	CuboidSet& closedList = memo.closedList;
	closedList.clear();
	closedList.reserve(current.volume());
	// Collect possible entry points from previous to current and add to openList.
	Cuboid overlap = previous.intersection(inflatedCurrent);
	Space& space = area.getSpace();
	for(const Point3D point : overlap)
		if(space.shape_canFitEver(point, shape, previous.getFacingTwordsOtherCuboid(current)))
		{
			openList.insert(point);
			closedList.add(point);
		}
	while(!openList.empty())
	{
		Point3D currentPoint = openList.back();
		openList.popBack();
		CuboidSet adjacentCuboids = CuboidSet::create(space.getAdjacentWithEdgeAndCornerAdjacent(currentPoint));
		adjacentCuboids.maybeRemoveAll(closedList);
		for(const Cuboid adjacentCuboid : adjacentCuboids)
		{
			for(Point3D adjacentPoint : adjacentCuboid)
			{
				if(!current.contains(adjacentPoint))
				{
					// The adjacentPoint is outside the bounds of the cuboid being searched. Record the output and return if all candidates have been confirmed.
					for(const Cuboid candidate : candidates)
						if(candidate.contains(adjacentPoint))
						{
							output.maybeInsert(candidate);
							if(output.size() == candidates.size())
								return output;
						}
				}
				else
				{
					// The adjacentPoint is inside the cuboid being searched. Find adjacent cuboids to add to long range open list.
					if constexpr(detour)
					{
						// If detouring we must check that the shape can fit currently.
						if(space.shape_canFitCurrentlyDynamic(adjacentPoint, shape, currentPoint.getFacingTwords(adjacentPoint), occupied))
						{
							openList.insert(adjacentPoint);
							closedList.add(adjacentPoint);
						}
					}
					// No detour, check if the shape fits ever.
					else if(space.shape_canFitEver(adjacentPoint, shape, currentPoint.getFacingTwords(adjacentPoint)))
					{
						openList.insert(adjacentPoint);
						closedList.add(adjacentPoint);
					}
				}
			}
		}
	}
	return output;
}
// Returns destination, point before destinaiton, and target. Returning point before allows the cuboid path to point path converter to path to the point before and then append the destinaiton, thus preserving the facing which was being used when finding the target.
template<longRangePath::ShortRangeCondition ConditionT, bool detour>
std::tuple<Point3D, Point3D, Point3D> longRangePath::getAccessableDestination(const Area& area, const auto& rtree, const Cuboid cuboid, const Cuboid previous, const ShapeId shape, ConditionT&& condition, ShortRangeMemo& memo, const CuboidSet& occupied)
{
	std::vector<Point3D>& openList = memo.openList;
	openList.clear();
	CuboidSet& closedList = memo.closedList;
	closedList.clear();
	Cuboid overlap = previous.intersection(cuboid.inflated({1}));
	Space& space = area.getSpace();
	for(const Point3D point : overlap)
		if(space.shape_canFitEver(point, shape, previous.getFacingTwordsOtherCuboid(cuboid)))
		{
			openList.insert(point);
			closedList.add(point);
		}
	while(!openList.empty())
	{
		Point3D current = openList.back();
		openList.pop_back();
		CuboidSet adjacent = CuboidSet::create(space.getAdjacentWithEdgeAndCornerAdjacent(current).intersection(cuboid));
		adjacent.maybeRemoveAll(closedList);
		for(const Cuboid adjacentCuboid : adjacent)
			for(const Point3D adjacentPoint : adjacentCuboid)
			{
				if constexpr(detour)
				{
					// If detouring we must check that the shape can fit currently.
					if(space.shape_canFitCurrentlyDynamic(adjacentPoint, shape, current.getFacingTwords(adjacentPoint), occupied))
					{
						Point3D result = condition(adjacentPoint, current.getFacingTwords(adjacentPoint));
						if(result.exists())
							return {adjacentPoint, current, result};
						openList.push_back(adjacentPoint);
						closedList.add(adjacentPoint);
					}

				}
				// No detour, check if the shape fits ever.
				else if(space.shape_canFitEver(adjacentPoint, shape, current.getFacingTwords(adjacentPoint)))
				{
					Point3D result = condition(adjacentPoint, current.getFacingTwords(adjacentPoint));
					if(result.exists())
						return {adjacentPoint, current, result};
					openList.push_back(adjacentPoint);
					closedList.add(adjacentPoint);
				}
			}
	}
	return {Point3D::null(), Point3D::null(), Point3D::null()};
}
// These three functions dispatch to different template specializations depending on run time bool arguments.
template<bool depthFirst, bool singleTile, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPathDetour(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition,  LongRangeMemo& memo, const bool detour, const Point3D huristicDestination)
{
	if(detour)
		return getPathInnerLoop<depthFirst, singleTile, true>(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, huristicDestination);
	else
		return getPathInnerLoop<depthFirst, singleTile, false>(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, huristicDestination);
}
template<bool depthFirst, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPathSingleTile(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition,  LongRangeMemo& memo, const bool singleTile, const bool detour, const Point3D huristicDestination)
{
	if(singleTile)
		return getPathDetour<depthFirst, true>(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, detour, huristicDestination);
	else
		return getPathDetour<depthFirst, false>(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, detour, huristicDestination);
}
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPathDepthFirst(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition,  LongRangeMemo& memo, const bool depthFirst, const bool singleTile, const bool detour, const Point3D huristicDestination)
{
	if(depthFirst)
		return getPathSingleTile<true>(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, singleTile, detour, huristicDestination);
	else
		return getPathSingleTile<false>(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, singleTile, detour, huristicDestination);
}
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPathAdjacent(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const bool depthFirst, const bool singleTile, const bool detour, const bool adjacent, const bool anyOccupied, const Distance maxRange, const Point3D huristicDestination)
{
	if(adjacent)
	{
		const Space& space = area.getSpace();
		Distance radiusPlusOne = Shape::getRadius(shape) + 1;
		auto wrappedLongRangeCondition = [&space, &longRangeCondition, radiusPlusOne](Cuboid cuboid) -> bool {
			// Inflate potential destination cuboid by radius plus 1 to check every point which could be adjacent from somewhere in this cuboid by shape,
			cuboid.inflate(radiusPlusOne);
			return longRangeCondition(cuboid);
		};
		auto wrappedShortRangeCondition = [&space, &shape, &shortRangeCondition](const Point3D point, const Facing4 facing) -> Point3D {
			CuboidSet adjacentPoints = Shape::getCuboidsWhichWouldBeAdjacentAt(shape, space, point, facing);
			for(const Cuboid cuboid : adjacentPoints)
			{
				auto target = shortRangeCondition(cuboid);
				if(target.exists())
					return target;
			}
			return Point3D::null();
		};
		return getPathMaxRange(area, rtree, start, startFacing, shape, occupied, wrappedLongRangeCondition, wrappedShortRangeCondition, memo, depthFirst, singleTile, detour, maxRange, huristicDestination);
	}
	// SingleTile has no effect in combination with occupied.
	else if(anyOccupied && !singleTile)
	{
		const Space& space = area.getSpace();
		Distance radius = Shape::getRadius(shape);
		auto wrappedLongRangeCondition = [&space, &longRangeCondition, radius](Cuboid cuboid) -> bool {
			// Inflate potential destination cuboid by radius plus 1 to check every point which could be occupied from somewhere in this cuboid by shape,
			cuboid.inflate(radius);
			return longRangeCondition(cuboid);
		};
		auto wrappedShortRangeCondition = [&space, &shape, &shortRangeCondition](const Point3D point, const Facing4 facing) -> Point3D {
			CuboidSet occupiedPoints = Shape::getCuboidsOccupiedAt(shape, space, point, facing);
			for(const Cuboid cuboid : occupiedPoints)
			{
				auto target = shortRangeCondition(cuboid);
				if(target.exists())
					return target;
			}
			return Point3D::null();
		};
		return getPathMaxRange(area, rtree, start, startFacing, shape, occupied, wrappedLongRangeCondition, wrappedShortRangeCondition, memo, depthFirst, singleTile, detour, maxRange, huristicDestination);
	}
	else
		// Not adjacent or occupied, location and target are the same point.
		return getPathMaxRange(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, depthFirst, singleTile, detour, maxRange, huristicDestination);
}
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPathMaxRange(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const bool depthFirst, const bool singleTile, const bool detour, const bool adjacent, const bool anyOccupied, const Distance maxRange, const Point3D huristicDestination)
{
	if(maxRange.empty())
		return getPathAdjacent(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, depthFirst, singleTile, detour, adjacent, anyOccupied, maxRange, huristicDestination);
	auto wrappedShortRangeCondition = [&](const Point3D point, const Facing4 facing) -> Point3D
	{
		if(point.distanceTo(start) > maxRange)
			return Point3D::null;
		return shortRangeCondition(point, facing);
	};
	auto wrappedLongRangeCondition = [&](const Cuboid cuboid) -> bool
	{
		if(cuboid.distanceTo(start) > maxRange)
			return false;
		return longRangeCondition(cuboid);
	};
	return getPathAdjacent(area, rtree, start, startFacing, shape, occupied, wrappedLongRangeCondition, wrappedShortRangeCondition, memo, depthFirst, singleTile, detour, adjacent, anyOccupied, maxRange, huristicDestination);
}
// Entry point.
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> longRangePath::getPath(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const FactionId faction, const bool depthFirst, const bool singleTile, const bool detour, const bool adjacent, const bool anyOccupied, const Distance maxRange, const Point3D huristicDestination)
{
	const Space& space = area.getSpace();
	if(faction.empty())
		return getPathMaxRange(area, rtree, start, startFacing, shape, occupied, longRangeCondition, shortRangeCondition, memo, depthFirst, singleTile, detour, adjacent, anyOccupied, maxRange, huristicDestination);
	auto wrappedShortRangeCondition = [&space, &rtree, &shortRangeCondition, shape, faction](const Point3D point, const Facing4 facing) {
		//TODO:(optimization) Would it be better if reservation filter was applied before shortRangeCondition? Maybe it should be configurable?
		Point3D target = shortRangeCondition(point, facing);
		if(target.empty())
			return Point3D::null();
		//TODO:(optimization) This call is run twice if occupied is true.
		CuboidSet toBeOccupied = Shape::getCuboidsOccupiedAt(shape, space, point, facing);
		if(space.isReservedAny(toBeOccupied, faction))
			return Point3D::null();
		else
			return target;
	};
	return getPathMaxRange(area, rtree, start, startFacing, shape, occupied, longRangeCondition, wrappedShortRangeCondition, memo, depthFirst, singleTile, detour, adjacent, anyOccupied, maxRange, huristicDestination);
}
template<bool detour>
SmallSet<Point3D> longRangePath::cuboidPathToPointPath(Area& area, ShapeId shape, const CuboidSet& occupied, const SmallSet<Cuboid> cuboids, Point3D start, Point3D end, Point3D pointBeforeEnd, ShortRangeMemo& memo)
{
	SmallSet<Point3D>& openList = memo.openList;
	openList.clear();
	openList.insert(start);
	// ClosedList is faster for excluding points but history is needed to reconstruct path.
	SmallMap<Point3D, Point3D> closedListHistory;
	CuboidSet closedList;
	closedListHistory.insert(start, Point3D::null());
	closedList.add(start);
	Space& space = area.getSpace();
	const CuboidSet cuboidSet = CuboidSet::create(cuboids);
	while(!openList.empty())
	{
		openList.sortUnaryDescending([&](const Point3D point) { return point.distanceTo(end); });
		Point3D current = openList.back();
		openList.popBack();
		// TODO:(optimization) this checks every cuboid against adjacent to current but only two cuboids are needed: the one that contains current and the one after it.
		CuboidSet adjacent = cuboidSet.intersection(current.inflated({1}));
		adjacent.maybeRemoveAll(closedList);
		for(const Cuboid cuboid : adjacent)
			for(const Point3D point : cuboid)
			{
				if constexpr(detour)
				{
					if(!space.shape_canEnterCurrentlyFrom(point, shape, current, occupied))
						continue;
				}
				else if(!space.shape_canFitEver(point, shape, current.getFacingTwords(point)))
						continue;
				if(point == pointBeforeEnd)
				{
					// Destination found, walk closed list and reconstruct path.
					SmallSet<Point3D> output;
					// TODO: magic number.
					output.reserve(start.distanceTo(end).get() * 2);
					while(current != start)
					{
						output.insert(current);
						current = closedListHistory[current];
					}
					return output;
				}
				closedListHistory.insert(point, current);
				closedList.add(point);
				openList.insert(point);
			}
	}
	// We already know a path exists from long range testing.
	assert(false);
}