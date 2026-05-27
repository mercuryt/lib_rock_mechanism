#pragma once
#include "longRange.h"
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/linePath.h"
#include "../geometry/rectangle.h"
#include "../space/space.h"
#include "../area/area.h"
#include <ranges>
template<bool depthFirst, bool singleTile, bool detour, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
longRangePath::IntermediateResult longRangePath::getPathInnerLoop(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	LongRangeMemo& memo = getMemo();
	// TODO: if cuboid is narrower then width in x or y prove a path for shape from previous cuboid to next cuboid before adding to openList.
	if constexpr(depthFirst)
		assert(params.huristicDestination.exists());
	if constexpr(detour)
		assert(depthFirst);
	const Cuboid startCuboid = rtree.queryGetLeaf(params.start);
	assert(startCuboid.exists());
	// Check if short range condition is true for starting position and facing.
	Point3D targetFoundForStartingPosition = shortRangeCondition(params.start, params.startFacing);
	if(targetFoundForStartingPosition.exists())
		return {std::vector<Cuboid>{startCuboid}, targetFoundForStartingPosition};
	std::vector<OpenListData>& openList = memo.openList;
	openList.clear();
	ClosedListLongRange& closedListToFrom = memo.closedListToFrom;
	closedListToFrom.clear();
	bool useShortRangeForStart = checkCuboidNotFreelyNavigable(startCuboid, params);
	if constexpr(detour)
	{
		const Space& space = params.area.getSpace();
		if(!useShortRangeForStart && space.shape_queryAnyDynamic(startCuboid))
			useShortRangeForStart = true;
	}
	openList.emplace_back(startCuboid, Cuboid::null(), 0, 0, useShortRangeForStart);
	if(!useShortRangeForStart)
		closedListToFrom.initalize(startCuboid, {});
	// If useShortRangeForStart is false both adjacentAndShortRange and adjacet are left empty.
	while(!openList.empty())
	{
		auto iter = std::ranges::min_element(openList, {}, &OpenListData::score);
		const Cuboid previous = iter->previous;
		const Cuboid current = iter->current;
		const int cost = iter->cost;
		const bool shortRangeEnabled = iter->shortRange;
		if(previous.empty())
			assert(closedListToFrom.m_data.empty() == shortRangeEnabled);
		(*iter) = openList.back();
		openList.pop_back();
		// Check if this item has been made redundant by another item creating the node this would be creating.
		// Only short range nodes are checked because long range nodes are added to open list at the same time as closed list so they cannot be redundant in the open list.
		if(shortRangeEnabled && closedListToFrom.checkWithFrom(current, previous))
			continue;
		// Check destination condition.
		if(longRangeCondition(current))
		{
			// There is a potential destination reachable from some point(s) in this cuboid.
			Point3D target = getAccessableDestination<singleTile, detour>(current, previous, shortRangeCondition, memo.shortRangeMemo, params);
			if(target.exists())
			{
				// If we don't need a point path then we don't need a cuboid path either.
				return {
					(params.returnPath ? closedListToFrom.getPath(current, previous) : std::vector<Cuboid>()),
					target
				};
			}
		}
		// Gather all adjacent.
		// Get short range here as well because it needs to be set when the portal dimensions can't be fit through, but we need to check portal dimensions now rather then in withEach to determine if long range needs to call checkPortalPathability.
		SmallMap<Cuboid, bool> adjacentAndShortRange;
		if(shortRangeEnabled)
			adjacentAndShortRange = getAccessableCuboidsShortRange<singleTile, detour>(rtree, current, previous, params, memo.shortRangeMemo);
		else
			adjacentAndShortRange = getAccessableCuboidsLongRange<singleTile, detour>(rtree, current, params);
		// Record in open list.
		for(const auto [candidate, useShortRangeForCandidate] : adjacentAndShortRange)
			withEachAdjacentCuboid<depthFirst, singleTile, detour>(candidate, current, cost, useShortRangeForCandidate, memo, params);
		if(shortRangeEnabled)
		{
			// Record all accessable adjacent in closed list.
			// This can be used to reconstruct the path despite a single cuboid appearing multiple times.
			// Instead of being a single cuboid like a long range node a short range node is a cuboid plus the set of adjacent cuboids which the current pather can transit through.
			// Long range is added to closed list in forEach, which is more efficent due to preventing redundant open list items but can't be done with short range because adjacent isn't known yet.
			auto view = adjacentAndShortRange.m_data | std::views::elements<0>;
			SmallSet<Cuboid> adjacent;
			adjacent.m_data = std::vector<Cuboid>(view.begin(), view.end());
			if(previous.empty())
				// This is the first cuboid
				closedListToFrom.initalize(current, std::move(adjacent));
			else
				closedListToFrom.close(current, previous, std::move(adjacent));
			//TODO: would it be worth it to purge or upgrade any short range nodes with current as cuboid?
		}
	}
	// No route found.
	return {{}, Point3D::null()};
}
// A helper for getPathInnerLoop.
// Applies closed list, calculates cost and score, and adds to memo.openList.
template<bool depthFirst, bool singleTile, bool detour>
void longRangePath::withEachAdjacentCuboid(const Cuboid cuboid, const Cuboid previous, const int cost, const bool useShortRange, LongRangeMemo& memo, const PathParamaters& params)
{
	assert(previous.exists());
	std::vector<OpenListData>& openList = memo.openList;
	ClosedListLongRange& closedListToFrom = memo.closedListToFrom;
	// Filter with closed list.
	if(useShortRange)
	{
		// Check the closed list for this combination of from / to.
		if(closedListToFrom.checkWithFrom(cuboid, previous))
			return;
		// Short range cuboids are not added to closed list yet because we need to know what adjacent cuboids are accessable.
		// For this reason we can't gurentee there won't be redundant items on the open list and so we have to filter each short range cuboid as it comes off the open list.
	}
	else
	{
		// If short range is disabled we only need to check 'to'. 'From' doesn't matter becuase every point in cuboid is accessable.
		if(closedListToFrom.checkWithoutFrom(cuboid))
			return;
		// Non short range cuboids can be added to closed list here because we don't need to know their accessable adjacent.
		closedListToFrom.close(cuboid, previous, {});
	}
	// Calculate cost and score.
	int newCost = cost;
	if(previous.exists())
		newCost += previous.distanceTo(cuboid).get();
	else
		newCost += params.start.distanceTo(cuboid).get();
	int score = newCost;
	// If depthFirst is set, we have a huristic destination to use so add it's distance to score.
	// if depthFirst is not set, score is equal to cost.
	if constexpr(depthFirst)
		score += cuboid.getCenter().distanceTo(params.huristicDestination).get();
	// Add to open list.
	assert(previous.exists());
	openList.emplace_back(cuboid, previous, newCost, score, useShortRange);
}
// Find which cuboids adjacent to current can be entered. Long range pathing to be used when Cuboid is freely navigable and shape enters through an unconstrained portal.
template<bool singleTile, bool detour>
SmallMap<Cuboid, bool> longRangePath::getAccessableCuboidsLongRange(const Enterable& rtree,const Cuboid current, const PathParamaters& params)
{
	SmallMap<Cuboid, bool> output;
	const Space& space = params.area.getSpace();
	const MoveTypeId moveType = params.moveType;
	rtree.queryForEach(current.inflated({1}), [&space, &output, &params, current, moveType](const Cuboid candidate){
		if(candidate == current)
			return;
		if(!space.move_cuboidCanBeEnteredFrom(current, candidate, moveType))
			return;
		bool useShortRange = false;
		if constexpr(!singleTile)
		{
			if(!checkPortalSize(candidate, current, params))
			{
				// Fast boundry check failed, try a detail check.
				if(!checkPortalPathabality<detour, singleTile>(candidate, current, params))
					return;
				// Since the shape can't fit through the portal we must do short range pathing.
				useShortRange = true;
			}
		}
		if constexpr(detour)
			if(!useShortRange && space.shape_queryAnyDynamic(candidate))
				useShortRange = true;
		output.insert(candidate, useShortRange);
	});
	return output;
}
// Find which cuboids adjacent to current can be entered. Short range pathing to be used when Cuboid is not freely navigable or shape enters through a constrained portal.
// Also returns if short range pathing should be used on the candidate.
template<bool singleTile, bool detour>
SmallMap<Cuboid, bool> longRangePath::getAccessableCuboidsShortRange(const Enterable& rtree,const Cuboid current, const Cuboid previous, const PathParamaters& params, ShortRangeMemo& memo)
{
	const Space& space = params.area.getSpace();
	Cuboid inflatedCurrent = current.inflated({1});
	CuboidSet candidatesCuboidSet = rtree.queryGetLeaves(inflatedCurrent);
	candidatesCuboidSet.remove(current);
	SmallSet<Cuboid> candidates = candidatesCuboidSet.m_cuboids;
	SmallMap<Cuboid, bool> output;
	std::vector<Point3D>& openList = memo.openList;
	openList.clear();
	CuboidSet& closedList = memo.closedList;
	closedList.clear();
	if(previous.empty())
	{
		// This is the first cuboid, seed the openList with the starting position instead of border with previous.
		openList.push_back(params.start);
		closedList.add(params.start);
	}
	else
	{
		// Collect possible entry points from previous to current and add to openList.
		Cuboid overlap = previous.intersection(inflatedCurrent);
		for(const Point3D point : overlap)
		// TODO: detour must check if point is currently enterable.
			if(space.shape_canFitEver(point, params.shape, previous.getFacing4TwordsOtherCuboid(current)))
			{
				openList.push_back(point);
				closedList.add(point);
			}
	}
	while(!openList.empty())
	{
		Point3D currentPoint = openList.back();
		openList.pop_back();
		CuboidSet adjacentCuboids = rtree.queryGetIntersection(currentPoint.inflated({1}));
		adjacentCuboids.maybeRemoveAll(closedList);
		for(const Cuboid adjacentCuboid : adjacentCuboids)
		{
			for(Point3D adjacentPoint : adjacentCuboid)
			{
				if(!shapeCanEnterFrom<detour, singleTile>(adjacentPoint, currentPoint, params))
					continue;
				if(!current.contains(adjacentPoint))
				{
					// The adjacentPoint is outside the bounds of the cuboid being searched. Record the output and return if all candidates have been confirmed.
					for(const Cuboid candidate : candidates)
						if(candidate.contains(adjacentPoint))
						{
							// Checking pathability of cuboid after checking pathability of point is somewhat strange, but point check doesn't take climb into account.
							if(!space.move_cuboidCanBeEnteredFrom(current, candidate, params.moveType))
								continue;
							// Found candidate for adjacentPoint.
							// Record short range as true if the portal isn't large enough for the shape to pass through or the cuobid is too small for the shape to turn.
							bool useShortRangeForCandidate = false;
							if constexpr(detour)
							{
								if(space.shape_queryAnyDynamic(candidate))
									useShortRangeForCandidate = true;
								else if constexpr(!singleTile)
									useShortRangeForCandidate = !checkPortalSize(candidate, current, params) || checkCuboidNotFreelyNavigable(candidate, params);
							}
							else if constexpr(!singleTile)
								useShortRangeForCandidate = !checkPortalSize(candidate, current, params) || checkCuboidNotFreelyNavigable(candidate, params);
							output.maybeInsert(candidate, useShortRangeForCandidate);
							if(output.size() == candidates.size())
								// All candidates found.
								return output;
							break;
						}
				}
				else
				{
					// AdjacentPoint is within bounds, continue expanding.
					openList.push_back(adjacentPoint);
					closedList.add(adjacentPoint);
				}
			}
		}
	}
	// Not all candidates have been found, return those which have.
	return output;
}
// Returns target to be used as huristic destination in final path building.
template<bool singleTile, bool detour, longRangePath::ShortRangeCondition ConditionT>
Point3D longRangePath::getAccessableDestination(const Cuboid cuboid, const Cuboid previous, ConditionT&& condition, ShortRangeMemo& memo, const PathParamaters& params)
{
	const Space& space = params.area.getSpace();
	std::vector<Point3D>& openList = memo.openList;
	openList.clear();
	// TODO: use CuboidSet for single tile shapes.
	ClosedListWithFacing& closedList = memo.closedListWithFacing;
	closedList.clear();
	if(previous.empty())
	{
		// This is the starting cuboid.
		openList.push_back(params.start);
		closedList.close(params.start, params.startFacing);
	}
	else
	{
		// Collect possible entry points.
		Cuboid overlap = previous.intersection(cuboid.inflated({1}));
		Facing4 facingTwordsOtherCuboid = previous.getFacing4TwordsOtherCuboid(cuboid);
		for(const Point3D point : overlap)
			if(space.shape_canFitEver(point, params.shape, facingTwordsOtherCuboid))
			{
				openList.push_back(point);
				if constexpr(singleTile)
					closedList.closeAllDirections(point);
				else
					closedList.close(point, facingTwordsOtherCuboid);
			}
	}
	while(!openList.empty())
	{
		// Try to find the nearest even though the path will be discarded, the end point will still be used.
		std::vector<Point3D>::iterator best;
		//TODO:(optimization) this could be a template paramater.
		if(previous.exists())
			best = std::ranges::min_element(openList, {}, [&](const Point3D point) { return point.distanceTo(previous); });
		else
			best = std::ranges::min_element(openList, {}, [&](const Point3D point) { return point.distanceTo(params.start); });
		Point3D current = *best;
		(*best) = openList.back();
		openList.pop_back();
		// Check destination condition in adjacent loop rather then here because the starting positions can never be destinations: they would have already been found when checking the previous cuboid or params.start.
		Cuboid adjacent = current.inflated({1}).intersection(cuboid);
		CuboidSet adjacentFiltered = CuboidSet::create(adjacent);
		closedList.removeFrom(adjacentFiltered);
		adjacentFiltered.maybeRemove(current);
		for(const Cuboid adjacentCuboid : adjacentFiltered)
			for(const Point3D adjacentPoint : adjacentCuboid)
			{
				assert(cuboid.contains(adjacentPoint));
				Facing4 facing = current.getFacingTwords(adjacentPoint);
				// Check closed list for this combination of point and facing.
				if(closedList.check(adjacentPoint, facing))
					continue;
				if(!shapeCanEnterFrom<detour, singleTile>(adjacentPoint, current, params))
					continue;
				Point3D result = condition(adjacentPoint, facing);
				if(result.exists())
					return result;
				openList.push_back(adjacentPoint);
				if constexpr(singleTile)
					closedList.closeAllDirections(adjacentPoint);
				else
					closedList.close(adjacentPoint, facing);
			}
	}
	return Point3D::null();
}
// These three functions dispatch to different template specializations depending on run time bool arguments.
template<bool depthFirst, bool singleTile, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] longRangePath::IntermediateResult longRangePath::getPathDetour(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	if(params.detour)
		return getPathInnerLoop<depthFirst, singleTile, true>(rtree, longRangeCondition, shortRangeCondition, params);
	else
		return getPathInnerLoop<depthFirst, singleTile, false>(rtree, longRangeCondition, shortRangeCondition, params);
}
template<bool depthFirst, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] longRangePath::IntermediateResult longRangePath::getPathSingleTile(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	if(params.singleTile)
		return getPathDetour<depthFirst, true>(rtree, longRangeCondition, shortRangeCondition, params);
	else
		return getPathDetour<depthFirst, false>(rtree, longRangeCondition, shortRangeCondition, params);
}
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] longRangePath::IntermediateResult longRangePath::getPathDepthFirst(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	if(params.depthFirst)
	{
		assert(params.huristicDestination.exists());
		return getPathSingleTile<true>(rtree, longRangeCondition, shortRangeCondition, params);
	}
	else
		return getPathSingleTile<false>(rtree, longRangeCondition, shortRangeCondition, params);
}
// Entry point.
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] PathResult longRangePath::getPath(const Enterable& rtree, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	// Coarse result first: cuboid path and target location.
	IntermediateResult intermediateResult = getPathUnreserved(rtree, longRangeCondition, shortRangeCondition, params);
	if(intermediateResult.target.empty())
		return {{}, Point3D::null()};
	if(!params.returnPath || intermediateResult.target == params.start)
		return {{}, intermediateResult.target};
	assert(!intermediateResult.cuboids.empty());
	assert(intermediateResult.cuboids.back().contains(params.start));
	// If a cuboid path was found then use it as boundry to generate a point path using the target as destination huristic.
	SmallSet<Point3D> path;
	//TODO: move the calls to cuboidPathtoPointPath to getPathUnreserved, so that detour and single tile are template paramaters.
	if(params.detour)
	{
		if(params.singleTile)
			path = cuboidPathToPointPath<true, true>(shortRangeCondition, params, intermediateResult, getMemo().shortRangeMemo);
		else
			path = cuboidPathToPointPath<false, true>(shortRangeCondition, params, intermediateResult, getMemo().shortRangeMemo);
	}
	else
	{
		if(params.singleTile)
			path = cuboidPathToPointPath<true, false>(shortRangeCondition, params, intermediateResult, getMemo().shortRangeMemo);
		else
			path = cuboidPathToPointPath<false, false>(shortRangeCondition, params, intermediateResult, getMemo().shortRangeMemo);
	}
	return {path, intermediateResult.target};
}
// Alternative entry point.
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCuboidCondition ShortRangeConditionT>
[[nodiscard]] PathResult longRangePath::getPathAdjacent(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	Distance radiusPlusOne = Shape::getRadius(params.shape) + 1;
	Distance height = Distance::create(Shape::getZSize(params.shape).get());
	auto wrappedLongRangeCondition = [&longRangeCondition, radiusPlusOne, height](Cuboid cuboid) -> bool {
		// Inflate potential destination cuboid horizontally by radius plus 1 to check every point which could be adjacent from somewhere in this cuboid by shape,
		// Also inflate low z by one and high z by shape height.
		cuboid.inflateHorizontal(radiusPlusOne);
		cuboid.m_high.setZ(cuboid.m_high.z() + height);
		if(cuboid.m_low.z() != 0)
			cuboid.m_low.setZ(cuboid.m_low.z() - 1);
		return longRangeCondition(cuboid);
	};
	const Space& space = params.area.getSpace();
	auto wrappedShortRangeCondition = [&space, &params, &shortRangeCondition](const Point3D point, const Facing4 facing) -> Point3D {
		CuboidSet adjacentPoints = Shape::getCuboidsOccupiedAndAdjacentAt(params.shape, space, point, facing);
		for(const Cuboid cuboid : adjacentPoints)
		{
			auto target = shortRangeCondition(cuboid);
			if(target.exists())
				return target;
		}
		return Point3D::null();
	};
	return getPath(rtree, wrappedLongRangeCondition, wrappedShortRangeCondition, params);
}
// Alternative entry point.
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCuboidCondition ShortRangeConditionT>
[[nodiscard]] PathResult longRangePath::getPathAnyOccupied(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	const Space& space = params.area.getSpace();
	Distance radius = Shape::getRadius(params.shape);
	Distance height = Distance::create(Shape::getZSize(params.shape).get());
	auto wrappedLongRangeCondition = [&space, &longRangeCondition, radius, height](Cuboid cuboid) -> bool {
		// Inflate potential destination cuboid by radius to check every point which could be occupied from somewhere in this cuboid by shape,
		// Inflate potential destination cuboid horizontally by radius plus 1 to check every point which could be adjacent from somewhere in this cuboid by shape,
		// Also inflate low z by one and set high z to shape height.
		cuboid.inflateHorizontal(radius);
		cuboid.m_high.setZ(cuboid.m_high.z() + height - 1);
		return longRangeCondition(cuboid);
	};
	auto wrappedShortRangeCondition = [&space, &params, &shortRangeCondition](const Point3D point, const Facing4 facing) -> Point3D {
		CuboidSet occupiedPoints = Shape::getCuboidsOccupiedAt(params.shape, space, point, facing);
		for(const Cuboid cuboid : occupiedPoints)
		{
			auto target = shortRangeCondition(cuboid);
			if(target.exists())
				return target;
		}
		return Point3D::null();
	};
	return getPath(rtree, wrappedLongRangeCondition, wrappedShortRangeCondition, params);
}
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] longRangePath::IntermediateResult longRangePath::getPathMaxRange(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	if(params.maxRange.empty())
		return getPathDepthFirst(rtree, longRangeCondition, shortRangeCondition, params);
	auto wrappedShortRangeCondition = [&](const Point3D point, const Facing4 facing) -> Point3D
	{
		if(point.distanceTo(params.start) > params.maxRange)
			return Point3D::null();
		return shortRangeCondition(point, facing);
	};
	auto wrappedLongRangeCondition = [&](const Cuboid cuboid) -> bool
	{
		if(cuboid.distanceTo(params.start) > params.maxRange)
			return false;
		return longRangeCondition(cuboid);
	};
	return getPathDepthFirst(rtree, wrappedLongRangeCondition, wrappedShortRangeCondition, params);
}
template<longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeCondition ShortRangeConditionT>
[[nodiscard]] longRangePath::IntermediateResult longRangePath::getPathUnreserved(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params)
{
	const Space& space = params.area.getSpace();
	if(params.faction.empty())
		return getPathMaxRange(rtree, longRangeCondition, shortRangeCondition, params);
	auto wrappedShortRangeCondition = [&space, &rtree, &shortRangeCondition, &params](const Point3D point, const Facing4 facing) {
		//TODO:(optimization) Would it be better if reservation filter was applied before shortRangeCondition? Maybe it should be configurable?
		Point3D target = shortRangeCondition(point, facing);
		if(target.empty())
			return Point3D::null();
		//TODO:(optimization) This call is run twice if detour is true.
		CuboidSet toBeOccupied = Shape::getCuboidsOccupiedAt(params.shape, space, point, facing);
		if(space.isReservedAny(toBeOccupied, params.faction))
			return Point3D::null();
		else
			return target;
	};
	return getPathMaxRange(rtree, longRangeCondition, wrappedShortRangeCondition, params);
}
template<bool singleTile, bool detour, longRangePath::ShortRangeCondition ShortRangeConditionT>
SmallSet<Point3D> longRangePath::cuboidPathToPointPath(ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params, const IntermediateResult& intermediateResult, ShortRangeMemo& memo)
{
	const std::vector<Cuboid>& cuboidPath = intermediateResult.cuboids;
	std::vector<std::pair<Point3D, int>>& openList = memo.openListWithCuboidIndex;
	openList.clear();
	// CuboidPath is backwards so open list starts at the end.
	openList.emplace_back(params.start, cuboidPath.size() - 1);
	ClosedListWithFacing& closedListWithFacing = memo.closedListWithFacing;
	closedListWithFacing.clear();
	closedListWithFacing.closeAllDirections(params.start);
	// ClosedList is faster for excluding points but history is needed to reconstruct path.
	std::vector<std::pair<Point3D, Point3D>>& history = memo.history;
	history.clear();
	history.emplace_back(params.start, Point3D::null());
	Distance closeToTarget{0};
	if(params.adjacent || params.anyOccupiedPoint)
		closeToTarget = Shape::getRadius(params.shape);
	if(params.adjacent)
		++closeToTarget;
	while(!openList.empty())
	{
		auto iter = std::ranges::min_element(openList, {}, [t = intermediateResult.target](const std::pair<Point3D, int> pointAndCuboidIndex) { return pointAndCuboidIndex.first.distanceToFractional(t); });
		Point3D current = iter->first;
		int cuboidIndex = iter->second;
		(*iter) = openList.back();
		openList.pop_back();
		Point3D previous = std::ranges::find(history, current, &std::pair<Point3D, Point3D>::first)->second;
		if(previous.exists() && shortRangeCondition(current, previous.getFacingTwords(current)).exists())
		{
			// Destination found, walk closed list and reconstruct path.
			SmallSet<Point3D> output;
			// TODO: magic number.
			output.reserve(params.start.distanceTo(current).get() * 2);
			while(current != params.start)
			{
				output.insert(current);
				current = std::ranges::find(history, current, &std::pair<Point3D, Point3D>::first)->second;
			}
			return output;
		}
		//TODO:(optimization) Make releventCuboids a CuboidArray<2>.
		CuboidSet releventCuboids = CuboidSet::create(cuboidPath[cuboidIndex]);
		// cuboidPath is ordered from end to start, so the relevent cuboids are the one that current is in, and possibly the one physicaly before (and logically after), if one exists.
		if(cuboidIndex != 0)
			// When cuboid at cuboidIndex is the physical first element there is no logical next. When it is not then that next cuboid is relevent for adjacent check.
			releventCuboids.add(cuboidPath[cuboidIndex - 1]);
		// By intersecting with releventCuboids we trim adjacent and keep the pathing search within the bounds established by the cuboidSet.
		CuboidSet adjacent = releventCuboids.intersection(current.inflated({1}));
		closedListWithFacing.removeFrom(adjacent);
		for(const Cuboid cuboid : adjacent)
			for(const Point3D point : cuboid)
			{
				if(!shapeCanEnterFrom<detour, singleTile>(point, current, params))
					continue;
				if constexpr(!singleTile)
				{
					// If near to the target close only tested facings, otherwise close all facings.
					if(point.distanceTo(intermediateResult.target) <= closeToTarget)
					{
						Facing4 facing = current.getFacingTwords(point);
						if(closedListWithFacing.checkAndCloseIfNot(point, facing))
							continue;
					}
					else
						closedListWithFacing.closeAllDirections(point);
				}
				else
					// Close all facing regardless of distance for singleTile.
					closedListWithFacing.closeAllDirections(point);
				history.emplace_back(point, current);
				if(cuboidPath[cuboidIndex].contains(point))
					openList.emplace_back(point, cuboidIndex);
				else
				{
					// If point is not contained in current cuboid it must be contained in logical next (physical previous) cuboid.
					assert(cuboidPath[cuboidIndex - 1].contains(point));
					openList.emplace_back(point, cuboidIndex - 1);
				}
			}
	}
	// We already know a path exists from long range testing.
	assert(false);
}
template<bool detour, bool singleTile>
bool longRangePath::shapeCanEnterFrom(const Point3D to, const Point3D from, const PathParamaters& params)
{
	const Space& space = params.area.getSpace();
	Facing4 facing = from.getFacingTwords(to);
	CuboidSet occupiedAtTo;
	if constexpr(!singleTile)
	{
		if(!Shape::isInBoundsAt(params.shape, space, to, facing))
			return false;
		occupiedAtTo = Shape::getCuboidsOccupiedAt(params.shape, space, to, facing);
		// Check shape can fit at 'to'.
		// Exclude dynamic solid and features from the check if not detouring.
		if constexpr(!detour)
		{
			CuboidSet occupiedAtToExcludingDynamic = occupiedAtTo;
			space.shape_queryRemoveFromDynamic(occupiedAtToExcludingDynamic);
			if(space.solid_isAny(occupiedAtToExcludingDynamic) || space.pointFeature_blocksEntrance(occupiedAtToExcludingDynamic))
				return false;
		}
		else
		{
			// Exclude self from the check if detouring.
			//TODO:(optimization) this should only be run for constructed items.
			CuboidSet occupiedAtToExcludingSelf = occupiedAtTo;
			occupiedAtToExcludingSelf.maybeRemoveAll(params.occupied);
			if(space.solid_isAny(occupiedAtToExcludingSelf) || space.pointFeature_blocksEntrance(occupiedAtToExcludingSelf))
				return false;
		}
		Cuboid occupiedBoundry = occupiedAtTo.boundry();
		if(occupiedBoundry.sizeZ() != 1)
		{
			occupiedBoundry.m_low.setZ(occupiedBoundry.m_low.z() + 1);
			CuboidSet occupiedWithoutBottomLayer = occupiedAtTo.intersection(occupiedBoundry);
			if(!space.pointFeature_multiTileCanEnterAtNonZeroZOffset(occupiedWithoutBottomLayer))
				return false;
		}
	}
	// Checks to ensure actors don't path through partitions.
	if(to.z() != from.z())
	{
		// If singleTile is true occupiedAtTo was never set. Set it here.
		// TODO:(optimization) Write a path for singleTile that uses a point instead.
		if constexpr(singleTile)
			occupiedAtTo.add(to);
		else if(occupiedAtTo.empty())
			occupiedAtTo = Shape::getCuboidsOccupiedAt(params.shape, space, to, facing);
		CuboidSet flattened = occupiedAtTo.flattened(to.z());
		if(to.z() < from.z())
			// Going down, shift up one to check for floors above the destination.
			// Otherwise we are going up, so check for floors at destination.
			flattened.shift({0, 0, 1}, {1});
		if(!space.pointFeature_canEnterFromBelowAll(flattened))
			return false;
	}
	if constexpr(detour)
	{
		if(!space.shape_canFitCurrentlyDynamic(to, params.shape, facing, params.occupied))
			return false;
	}
	return true;
}
template<bool detour, bool singleTile>
bool longRangePath::shapeCanEnterWithFacing(const Point3D to, const Facing4 facing, const PathParamaters& params)
{
	const Space& space = params.area.getSpace();
	if constexpr(!singleTile)
	{
		// Check shape is fully in bounds at point with facing.
		OffsetCuboid boundryAtTo = Shape::getOffsetCuboidBoundryWithFacing(params.shape, facing);
		boundryAtTo.shift(to.toOffset(), {1});
		if(!space.offsetBoundry().contains(boundryAtTo))
			return false;
		CuboidSet occupiedAtTo = Shape::getCuboidsOccupiedAt(params.shape, space, to, facing);
		// Check shape can fit at 'to'.
		if(space.solid_isAny(occupiedAtTo) || space.pointFeature_blocksEntrance(occupiedAtTo))
			return false;
		Cuboid occupiedBoundry = occupiedAtTo.boundry();
		if(occupiedBoundry.sizeZ() != 1)
		{
			occupiedBoundry.m_low.setZ(occupiedBoundry.m_low.z() + 1);
			CuboidSet occupiedWithoutBottomLayer = occupiedAtTo.intersection(occupiedBoundry);
			if(space.pointFeature_multiTileCanEnterAtNonZeroZOffset(occupiedWithoutBottomLayer))
				return false;
		}
	}
	if constexpr(detour)
		return space.shape_canFitCurrentlyDynamic(to, params.shape, facing, params.occupied);
	return true;
}
template<bool detour, bool singleTile>
bool longRangePath::checkPortalPathabality(const Cuboid to, const Cuboid from, const PathParamaters& params)
{
	// Find a point in current which is adjacent to cuboid where shape can fit facing cuboid and can step forward.
	Cuboid fromPoints = to.inflated({1}).intersection(from);
	Facing4 facing = from.getFacing4TwordsOtherCuboid(to);
	for(const Point3D point : fromPoints)
	{
		// Check shape can fit at point when facing twords to.
		if(shapeCanEnterWithFacing<detour, singleTile>(point, facing, params))
		{
			Cuboid adjacent = point.inflated({1});
			for(const Point3D adjacentPoint : adjacent.intersection(to))
				// check possible destinations.
				if(shapeCanEnterFrom<detour, singleTile>(adjacentPoint, point, params))
					return true;
		}
	}
	return false;
}