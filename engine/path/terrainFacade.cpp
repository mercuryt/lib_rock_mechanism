#include "terrainFacade.hpp"
#include "actors/actors.h"
#include "area/area.h"
#include "callbackTypes.h"
#include "config.h"
#include "numericTypes/index.h"
#include "pathMemo.h"
#include "definitions/shape.h"
#include "simulation/simulation.h"
#include "numericTypes/types.h"
#include "util.h"
#include "pathRequest.h"
#include "findPathInnerLoops.h"
#include "space/space.h"
#include <algorithm>
#include <queue>
#include <iterator>

FindPathResult::FindPathResult(const SmallSet<Point3D>& p, Point3D btpp, bool ucp) :
	path(p), pointThatPassedPredicate(btpp), useCurrentPosition(ucp)
{
	validate();
}
void FindPathResult::validate() const
{
	if(pointThatPassedPredicate.exists())
		assert(!path.empty() || useCurrentPosition);
	if(path.empty() && !useCurrentPosition)
		assert(pointThatPassedPredicate.empty());
}
TerrainFacade::TerrainFacade(Area& area, const MoveTypeId& moveType) : m_area(area), m_moveType(moveType)
{
	Space& space = m_area.getSpace();
	Cuboid cuboid = space.getAll();
	for(const Point3D& point : cuboid)
		update(point);
	m_pointToIndexConversionMultipliers = {space.m_sizeX * space.m_sizeY * 26,space.m_sizeX * 26, Distance::create(26)};
	// Two results fit on a  cache line, by ensuring that the number of tasks per thread is a multiple of 2 we prevent false shareing.
	assert(Config::pathRequestsPerThread % 2 == 0);
}
void TerrainFacade::doStep()
{
	Actors& actors = m_area.getActors();
	// Read step.
	// Breadth first.
	uint numberOfRequests = m_pathRequestsNoHuristic.size();
	uint i = 0;
	std::vector<std::pair<uint, uint>> ranges;
	while(i < numberOfRequests)
	{
		uint end = std::min(numberOfRequests, i + Config::pathRequestsPerThread);
		ranges.emplace_back(i, end);
		i = end;
	}
	m_pathRequestsNoHuristic.sortBy([&](const PathRequestNoHuristicData& a, const PathRequestNoHuristicData& b){
		return a.spatialHash < b.spatialHash;
	});
	//#pragma omp parallel for
	for(auto [begin, end] : ranges)
	{
		auto pair = m_area.m_simulation.m_hasPathMemos.getBreadthFirst(m_area);
		auto& memo = *pair.first;
		auto point = pair.second;
		assert(memo.empty());
		for(uint i = begin; i < end; ++i)
		{
			PathRequestIndex pathRequestIndex = PathRequestIndex::create(i);
			auto& pathRequestData = m_pathRequestsNoHuristic[pathRequestIndex];
			// Nullify pointer to path request about to be processed.
			ActorIndex actorIndex = pathRequestData.pathRequest->actor.getIndex(actors.m_referenceData);
			if(actorIndex.exists())
				actors.move_pathRequestClear(actorIndex);
			pathRequestData.result = pathRequestData.pathRequest->readStep(m_area, *this, memo);
			pathRequestData.result.validate();
			memo.reset();
		}
		m_area.m_simulation.m_hasPathMemos.releaseBreadthFirst(point);
	}
	// Depth First.
	numberOfRequests = m_pathRequestsWithHuristic.size();
	i = 0;
	ranges.clear();
	while(i < numberOfRequests)
	{
		uint end = std::min(numberOfRequests, i + Config::pathRequestsPerThread);
		ranges.emplace_back(i, end);
		i = end;
	}
	m_pathRequestsWithHuristic.sortBy([&](const PathRequestWithHuristicData& a, const PathRequestWithHuristicData& b){
		return a.spatialHash < b.spatialHash;
	});
	//#pragma omp parallel for
	for(auto [begin, end] : ranges)
	{
		auto pair = m_area.m_simulation.m_hasPathMemos.getDepthFirst(m_area);
		auto& memo = *pair.first;
		auto point = pair.second;
		assert(memo.empty());
		for(uint i = begin; i < end; ++i)
		{
			PathRequestIndex pathRequestIndex = PathRequestIndex::create(i);
			auto& pathRequestData = m_pathRequestsWithHuristic[pathRequestIndex];
			// Nullify pointer to path request about to be processed.
			ActorIndex actorIndex = pathRequestData.pathRequest->actor.getIndex(actors.m_referenceData);
			if(actorIndex.exists())
				actors.move_pathRequestClear(actorIndex);
			pathRequestData.result = pathRequestData.pathRequest->readStep(m_area, *this, memo);
			pathRequestData.result.validate();
			memo.reset();
		}
		m_area.m_simulation.m_hasPathMemos.releaseDepthFirst(point);
	}
	// Write step.
	// Move the callback and result data so we can flush the data store and generate new requests for next step within callbacks.
	auto pathRequestsNoHuristic = std::move(m_pathRequestsNoHuristic);
	auto pathRequestsWithHuristic = std::move(m_pathRequestsWithHuristic);
	clearPathRequests();
	// Breadth First.
	for(PathRequestNoHuristicData& pathRequestData : pathRequestsNoHuristic)
		pathRequestData.pathRequest->writeStep(m_area, pathRequestData.result);
	// Depth First.
	for(PathRequestWithHuristicData& pathRequestData : pathRequestsWithHuristic)
		pathRequestData.pathRequest->writeStep(m_area, pathRequestData.result);
}
void TerrainFacade::clearPathRequests()
{
	m_pathRequestsNoHuristic.clear();
	m_pathRequestsWithHuristic.clear();
}
bool TerrainFacade::getValueForBit(const Point3D& from, const Point3D& to) const
{
	Space& space = m_area.getSpace();
	return to.exists() &&
		space.shape_anythingCanEnterEver(to) &&
		space.shape_moveTypeCanEnter(to, m_moveType) &&
		space.shape_moveTypeCanEnterFrom(to, m_moveType, from);
}
PathRequestIndex TerrainFacade::getPathRequestIndexNoHuristic()
{
	PathRequestIndex point = PathRequestIndex::create(m_pathRequestsNoHuristic.size());
	m_pathRequestsNoHuristic.resize(point + 1);
	return point;
}
PathRequestIndex TerrainFacade::getPathRequestIndexWithHuristic()
{
	PathRequestIndex point = PathRequestIndex::create(m_pathRequestsWithHuristic.size());
	m_pathRequestsWithHuristic.resize(point + 1);
	return point;
}
void TerrainFacade::registerPathRequestNoHuristic(std::unique_ptr<PathRequestBreadthFirst> pathRequest)
{
	m_pathRequestsNoHuristic.emplaceBack(std::move(pathRequest), pathRequest->start.hilbertNumber());
}
void TerrainFacade::registerPathRequestWithHuristic(std::unique_ptr<PathRequestDepthFirst> pathRequest)
{
	m_pathRequestsWithHuristic.emplaceBack(std::move(pathRequest), pathRequest->start.hilbertNumber());
}
void TerrainFacade::update(const Point3D& point)
{
	Space& space = m_area.getSpace();
	std::bitset<26> enterable;
	uint8_t index = 0;
	for(Point3D adjacent : point.getAllAdjacentIncludingOutOfBounds())
	{
		enterable.set(index, getValueForBit(point, adjacent));
		++index;
	}
	m_enterable.maybeInsertOrOverwrite(point, enterable);
}
void TerrainFacade::movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	m_pathRequestsNoHuristic[newIndex] = std::move(m_pathRequestsNoHuristic[oldIndex]);
}
void TerrainFacade::movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	m_pathRequestsWithHuristic[newIndex] = std::move(m_pathRequestsWithHuristic[oldIndex]);
}
void TerrainFacade::unregisterNoHuristic(PathRequest& pathRequest)
{
	auto found = std::ranges::find(m_pathRequestsNoHuristic, &pathRequest, [&](const PathRequestNoHuristicData& data) { return data.pathRequest.get();});
	assert(found != m_pathRequestsNoHuristic.end());
	m_pathRequestsNoHuristic.remove(found);
}
void TerrainFacade::unregisterWithHuristic(PathRequest& pathRequest)
{
	auto found = std::ranges::find(m_pathRequestsWithHuristic, &pathRequest, [&](const PathRequestWithHuristicData& data) { return data.pathRequest.get();});
	assert(found != m_pathRequestsWithHuristic.end());
	m_pathRequestsWithHuristic.remove(found);
}
bool TerrainFacade::canEnterFrom(const Point3D& point, AdjacentIndex adjacentCount) const
{
	Space& space = m_area.getSpace();
	return m_enterable.queryGetOne(point).test(adjacentCount.get());
}
FindPathResult TerrainFacade::findPathTo(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour, bool adjacent, const FactionId& faction) const
{
	auto destinationCondition = [target](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {point == target, point}; };
	constexpr bool anyOccupiedPoint = false;
	memo.setDestination(target);
	return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, Distance::max());
}
FindPathResult TerrainFacade::findPathToWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour, bool adjacent, const FactionId& faction) const
{
	auto destinationCondition = [target](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {point == target, point}; };
	constexpr bool anyOccupiedPoint = false;
	return findPathDepthFirstWithoutMemo<anyOccupiedPoint, decltype(destinationCondition)>(start, startFacing, destinationCondition, target, shape, m_moveType, detour, adjacent, faction, Distance::max());
}

FindPathResult TerrainFacade::findPathToAnyOf(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, SmallSet<Point3D> points, const Point3D huristicDestination, bool detour, const FactionId& faction) const
{

	auto destinationCondition = [points](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {points.contains(point), point}; };
	constexpr bool anyOccupiedPoint = true;
	constexpr bool adjacent = false;
	memo.setDestination(huristicDestination);
	return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, Distance::max());
}
FindPathResult TerrainFacade::findPathToAnyOfWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, SmallSet<Point3D> points, const Point3D huristicDestination, bool detour, const FactionId& faction) const
{
	auto destinationCondition = [huristicDestination, points](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {points.contains(point), point}; };
	constexpr bool anyOccupiedPoint = true;
	constexpr bool adjacent = false;
	return findPathDepthFirstWithoutMemo<anyOccupiedPoint>(start, startFacing, destinationCondition, huristicDestination, shape, m_moveType, detour, adjacent, faction, Distance::max());
}
FindPathResult TerrainFacade::findPathToSpaceDesignation(PathMemoBreadthFirst& memo, const SpaceDesignation designation, const FactionId& faction, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool adjacent, bool unreserved, const Distance& maxRange) const
{
	assert(faction.exists());
	auto destinationCondition = [](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {true, point}; };
	constexpr bool anyOccupiedPoint = true;
	return findPathToSpaceDesignationAndCondition<anyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, memo, designation, faction, start, startFacing, shape, detour, adjacent, unreserved, maxRange);
}
//TODO: this could dispatch to specific actor / item variants rather then checking actor or item twice.
FindPathResult TerrainFacade::findPathAdjacentToPolymorphicWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, bool detour) const
{
	SmallSet<Point3D> targets;
	Space& space = m_area.getSpace();
	auto source = actorOrItem.getAdjacentPoints(m_area);
	for(const Point3D& point : source)
		if(
			space.shape_anythingCanEnterEver(point) &&
			space.shape_moveTypeCanEnter(point, m_moveType) &&
			space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(point, shape, m_moveType)
		)
			targets.insert(point);
	if(targets.empty())
		return { };
	return findPathToAnyOfWithoutMemo(start, startFacing, shape, targets, actorOrItem.getLocation(m_area), detour);
}
FindPathResult TerrainFacade::findPathToEdge(PathMemoBreadthFirst& memo, const Point3D &start, const Facing4 &startFacing, const ShapeId &shape, bool detour) const
{
	Space& space = m_area.getSpace();
	auto destinationCondition = [&](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D>
	{
		return {space.isEdge(point), point};
	};
	constexpr bool useAnyOccupiedPoint = true;
	constexpr bool adjacent = false;
	constexpr FactionId faction;
	return findPathToConditionBreadthFirst<useAnyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, memo, start, startFacing, shape, detour, adjacent, faction, Distance::max());
}
bool TerrainFacade::accessable(const Point3D& start, const Facing4& startFacing, const Point3D& to, const ActorIndex& actor) const
{
	Actors& actors = m_area.getActors();
	auto result = findPathToWithoutMemo(start, startFacing, actors.getShape(actor), to);
	return !result.path.empty() || result.useCurrentPosition;
}
void AreaHasTerrainFacades::doStep()
{
	for(auto& pair : m_data)
		pair.second->doStep();
}
void AreaHasTerrainFacades::update(const Point3D& point)
{
	for(auto& pair : m_data)
		pair.second->update(point);
}
void AreaHasTerrainFacades::maybeRegisterMoveType(const MoveTypeId& moveType)
{
	//TODO: generate terrain facade in thread.
	auto found = m_data.find(moveType);
	if(found == m_data.end())
		m_data.emplace(moveType, m_area, moveType);
}
void AreaHasTerrainFacades::updatePointAndAdjacent(const Point3D& point)
{
	Space& space = m_area.getSpace();
	if(space.shape_anythingCanEnterEver(point))
		update(point);
	for(Point3D adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
		if(space.shape_anythingCanEnterEver(adjacent))
			update(adjacent);
}
void AreaHasTerrainFacades::clearPathRequests()
{
	for(auto& pair : m_data)
		pair.second->clearPathRequests();
}
TerrainFacade& AreaHasTerrainFacades::getForMoveType(const MoveTypeId& moveType)
{
	assert(m_data.contains(moveType));
	return m_data[moveType];
}
TerrainFacade& AreaHasTerrainFacades::getOrCreateForMoveType(const MoveTypeId& moveType)
{
	//TODO: generate terrain facade in thread.
	auto found = m_data.find(moveType);
	if(found == m_data.end())
		return m_data.emplace(moveType, m_area, moveType);
	else
		return *found->second;
}