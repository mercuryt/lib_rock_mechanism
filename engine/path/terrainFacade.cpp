#include "terrainFacade.hpp"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../callbackTypes.h"
#include "../config.h"
#include "../definitions/shape.h"
#include "../definitions/moveType.h"
#include "../numericTypes/index.h"
#include "../numericTypes/types.h"
#include "../simulation/simulation.h"
#include "../space/space.h"
#include "../space/adjacentOffsets.h"
#include "../util.h"
#include "findPathInnerLoops.h"
#include "pathMemo.h"
#include "pathRequest.h"
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
	if(!path.empty())
	{
		auto iter = path.end();
		while(--iter != path.begin())
			assert(iter->isAdjacentTo(*(iter - 1)));
	}
}
PathRequestNoHuristicData::PathRequestNoHuristicData(std::unique_ptr<PathRequestBreadthFirst> pr, uint32_t sh) :
	pathRequest(std::move(pr)),
	spatialHash(sh)
{ }
PathRequestNoHuristicData::PathRequestNoHuristicData(PathRequestNoHuristicData&& other) noexcept :
	pathRequest(std::move(other.pathRequest)),
	spatialHash(other.spatialHash)
{ }
PathRequestNoHuristicData& PathRequestNoHuristicData::operator=(PathRequestNoHuristicData&& other) noexcept
{
	pathRequest = std::move(other.pathRequest);
	spatialHash = other.spatialHash;
	return *this;
}
PathRequestWithHuristicData::PathRequestWithHuristicData(std::unique_ptr<PathRequestDepthFirst> pr, uint32_t sh) :
	pathRequest(std::move(pr)),
	spatialHash(sh)
{ }
PathRequestWithHuristicData::PathRequestWithHuristicData(PathRequestWithHuristicData&& other) noexcept :
	pathRequest(std::move(other.pathRequest)),
	spatialHash(other.spatialHash)
{ }
PathRequestWithHuristicData& PathRequestWithHuristicData::operator=(PathRequestWithHuristicData&& other) noexcept
{
	pathRequest = std::move(other.pathRequest);
	spatialHash = other.spatialHash;
	return *this;
}
TerrainFacade::TerrainFacade(Area& area, const MoveTypeId& moveType) : m_area(area), m_moveType(moveType)
{
	Space& space = m_area.getSpace();
	const auto swim = MoveType::getSwim(moveType);
	// An optimized m_enterable generator for surface movement types.
	if(MoveType::getSurface(moveType) && MoveType::getClimb(moveType) < 2)
	{
		CuboidSet candidates;
		Distance maxZ = space.m_sizeZ - 1;
		// Solid.
		const CuboidSet solidLeaves = space.getSolid().getLeafCuboids();
		for(const Cuboid& cuboid : solidLeaves)
			if(cuboid.m_high.z() != maxZ)
			{
				Cuboid above = cuboid.getFace(Facing6::Above);
				above.shift(Facing6::Above, {1});
				candidates.maybeAdd(above);
			}
		candidates.maybeRemoveAll(solidLeaves);
		// Features.
		const auto featureLeaves = space.getPointFeatures().queryGetAllWithCuboids(space.boundry());
		for(const auto& [cuboid, value] : featureLeaves)
		{
			const PointFeatureType& featureType = PointFeatureType::byId(value.pointFeatureType);
			if(featureType.canStandIn)
				candidates.maybeAdd(cuboid);
			if(featureType.canStandAbove)
			{
				Cuboid above = cuboid.getFace(Facing6::Above);
				above.shift(Facing6::Above, {1});
				candidates.maybeAdd(above);
			}
		}
		// Fluids.
		if(!swim.empty())
		{
			const auto condition = [&](const FluidData& fluidData) { return swim.contains(fluidData.type); };
			const CuboidSet fluidGroups = space.fluid_queryGetCuboidsWithCondition(space.boundry(), condition);
			candidates.maybeAddAll(fluidGroups);
		}
		for(const Cuboid& cuboid : candidates)
			update(cuboid, candidates);
	}
	else if(!swim.empty() && !MoveType::getFly(moveType))
	{
		// Swim and not fly.
		const auto condition = [&](const FluidData& fluidData) { return swim.contains(fluidData.type); };
		const CuboidSet fluidGroups = space.fluid_queryGetCuboidsWithCondition(space.boundry(), condition);
		for(const Cuboid& cuboid : fluidGroups)
			update(cuboid, fluidGroups);
	}
	else
	{
		Cuboid boundry = space.boundry();
		CuboidSet candidates = CuboidSet::create(boundry);
		CuboidSet cannotEnter = space.getSolid().getLeafCuboids();
		const auto featureCondition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).blocksEntrance; };
		CuboidSet allFeaturesWhichBlockEntrance = space.pointFeature_queryCuboids(boundry, featureCondition);
		cannotEnter.maybeAddAll(allFeaturesWhichBlockEntrance);
		CuboidSet allDynamic = space.getDynamic().queryGetLeaves(boundry);
		// Remove dynamic from cannotEnter: dynamic cuboids are only temporarily impassible.
		// Note that despite this deck space is 'permanantly' passible.
		cannotEnter.maybeRemoveAll(allDynamic);
		candidates.maybeRemoveAll(cannotEnter);
		for(const Cuboid& cuboid : candidates)
			update(cuboid, candidates);
	}
	m_pointToIndexConversionMultipliers = {space.m_sizeX * space.m_sizeY * 26, space.m_sizeX * 26, Distance::create(26)};
	m_enterable.prepare();
	// Two results fit on a cache line, by ensuring that the number of tasks per thread is a multiple of 2 we prevent false shareing.
	assert(Config::pathRequestsPerThread % 2 == 0);
}
void TerrainFacade::doStep()
{
	m_enterable.prepare();
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
		auto pair = m_area.m_simulation.m_hasPathMemos.getBreadthFirst();
		auto& memo = *pair.first;
		auto point = pair.second;
		assert(memo.empty());
		for(i = begin; i < end; ++i)
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
		auto pair = m_area.m_simulation.m_hasPathMemos.getDepthFirst();
		auto& memo = *pair.first;
		auto point = pair.second;
		assert(memo.empty());
		for(i = begin; i < end; ++i)
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
bool TerrainFacade::getValue(const Point3D& to, const Point3D& from) const
{
	Space& space = m_area.getSpace();
	return
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
AdjacentData TerrainFacade::makeDataForPoint(const Point3D& point) const
{
	[[maybe_unused]] Point3D debugPoint = Point3D::create(0,4,1);
	const Space& space = m_area.getSpace();
	assert(space.shape_anythingCanEnterEver(point));
	assert(space.shape_moveTypeCanEnter(point, m_moveType));
	const Cuboid boundry = space.boundry();
	assert(boundry.contains(point));
	AdjacentData data;
	const Offset3D pointOffset = point.toOffset();
	for(AdjacentIndex index = {0}; index != maxAdjacent; ++index)
	{
		Offset3D offset = adjacentOffsets::all[index.get()];
		offset += pointOffset;
		bool value = false;
		if(boundry.contains(offset))
		{
			Point3D adjacent = Point3D::create(offset);
			value =
				space.shape_anythingCanEnterEver(adjacent) &&
				space.shape_moveTypeCanEnter(adjacent, m_moveType) &&
				space.shape_moveTypeCanEnterFrom(adjacent, m_moveType, point);
		}
		data.set(index, value);
	}
	return data;
}
AdjacentData TerrainFacade::makeDataForPoint(const Point3D& point, const CuboidSet& candidates) const
{
	[[maybe_unused]] Point3D debugPoint = Point3D::create(6,6,0);
	const Space& space = m_area.getSpace();
	assert(space.shape_anythingCanEnterEver(point));
	assert(space.shape_moveTypeCanEnter(point, m_moveType));
	const Cuboid boundry = space.boundry();
	assert(boundry.contains(point));
	AdjacentData data;
	const Offset3D pointOffset = point.toOffset();
	const Cuboid adjacentCuboid = point.getAllAdjacentIncludingOutOfBounds();
	const CuboidSet enterableAdjacent = candidates.intersection(adjacentCuboid);
	for(AdjacentIndex index = {0}; index != maxAdjacent; ++index)
	{
		const Offset3D adjacent = adjacentOffsets::all[index.get()] + pointOffset;
		if(!enterableAdjacent.contains(adjacent))
			continue;
		const Point3D adjacentPoint = Point3D::create(adjacent);
		if(
			space.shape_anythingCanEnterEver(adjacentPoint) &&
			space.shape_moveTypeCanEnter(adjacentPoint, m_moveType) &&
			space.shape_moveTypeCanEnterFrom(Point3D::create(adjacent), m_moveType, point)
		)
			data.set(index, true);
	}
	return data;
}
void TerrainFacade::update(const Cuboid& cuboid)
{
	const Space& space = m_area.getSpace();
	assert(space.boundry().intersects(cuboid));
	m_enterable.maybeRemove(cuboid);
	for(const Point3D& point : cuboid)
		if(
			space.shape_anythingCanEnterEver(point) &&
			space.shape_moveTypeCanEnter(point, m_moveType)
		)
		{
			const AdjacentData data = makeDataForPoint(point);
			if(!data.empty())
				m_enterable.maybeInsert(point, data);
		}
}
void TerrainFacade::update(const Cuboid& cuboid, const CuboidSet& candidates)
{

	const Space& space = m_area.getSpace();
	assert(space.boundry().contains(cuboid));
	m_enterable.maybeRemove(cuboid);
	for(const Point3D& point : cuboid)
		if(
			space.shape_anythingCanEnterEver(point) &&
			space.shape_moveTypeCanEnter(point, m_moveType)
		)
		{
			const AdjacentData data = makeDataForPoint(point, candidates);
			if(!data.empty())
				m_enterable.maybeInsert(point, data);
		}
}
void TerrainFacade::maybeSetImpassable(const Cuboid& cuboid)
{
	m_enterable.maybeRemove(cuboid);
	// Inflate the cuboid to update the points which were previously touching it.
	Cuboid inflated = cuboid;
	inflated.inflate({1});
	update(m_area.getSpace().boundry().intersection(inflated));
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
void TerrainFacade::updatePoint(const Point3D& point)
{
	const Space& space = m_area.getSpace();
	if(!space.shape_anythingCanEnterEver(point))
	{
		m_enterable.maybeRemove(point);
		return;
	}
	m_enterable.maybeInsertOrOverwrite(point, makeDataForPoint(point));
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
FindPathResult TerrainFacade::findPathTo(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour, bool adjacent, const FactionId& faction) const
{
	constexpr bool anyOccupiedPoint = false;
	memo.setDestination(target);
	if(adjacent)
	{
		auto destinationCondition = [target](const Cuboid& cuboid) -> std::pair<bool, Point3D> { return {cuboid.contains(target), target}; };
		return PathInnerLoops::findPathAdjacentTo<PathMemoDepthFirst, decltype(destinationCondition), anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, Distance::max());
	}
	else
	{
		auto destinationCondition = [target](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {point == target, point}; };
		return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, Distance::max());
	}
}
template<bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour, const FactionId& faction) const
{
	if constexpr(adjacent || anyOccupiedPoint)
	{
		auto destinationCondition = [target](const Cuboid& cuboid) -> std::pair<bool, Point3D> { return {cuboid.contains(target), target}; };
		return findPathDepthFirstWithoutMemo<decltype(destinationCondition), anyOccupiedPoint, adjacent>(start, startFacing, destinationCondition, target, shape, m_moveType, detour, faction, Distance::max());
	}
	else
	{
		auto destinationCondition = [target](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {point == target, point}; };
		return findPathDepthFirstWithoutMemo<decltype(destinationCondition), false, false>(start, startFacing, destinationCondition, target, shape, m_moveType, detour, faction, Distance::max());
	}
}
template FindPathResult TerrainFacade::findPathToWithoutMemo<false, true>(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour, const FactionId& faction) const;
template FindPathResult TerrainFacade::findPathToWithoutMemo<true, false>(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour, const FactionId& faction) const;
template<bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToAnyOf(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const CuboidSet& points, const Point3D huristicDestination, bool detour, const FactionId& faction) const
{
	memo.setDestination(huristicDestination);
	if constexpr(adjacent)
	{
		auto destinationCondition = [points](const Cuboid& cuboid) -> std::pair<bool, Point3D>
		{
			if(points.intersects(cuboid))
				return {true, points.intersectionPoint(cuboid)};
			return {false, Point3D::null()};
		};
		return PathInnerLoops::findPathAdjacentTo<PathMemoDepthFirst, decltype(destinationCondition), anyOccupiedPoint>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, Distance::max());
	}
	else
	{
		auto destinationCondition = [points](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D> { return {points.contains(point), point}; };
		return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedPoint, decltype(destinationCondition)>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, faction, Distance::max());
	}
}
template FindPathResult TerrainFacade::findPathToAnyOf<false, false>(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const CuboidSet& points, const Point3D huristicDestination, bool detour, const FactionId& faction) const;
template FindPathResult TerrainFacade::findPathToAnyOf<false, true>(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const CuboidSet& points, const Point3D huristicDestination, bool detour, const FactionId& faction) const;
FindPathResult TerrainFacade::findPathToAnyOfWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const CuboidSet& points, const Point3D huristicDestination, bool detour, const FactionId& faction) const
{
	auto destinationCondition = [huristicDestination, points](const Cuboid& cuboid) -> std::pair<bool, Point3D>
	{
		for(const Cuboid& otherCuboid : points)
			if(cuboid.intersects(otherCuboid))
				return {true, cuboid.intersection(otherCuboid).m_high};
		return {false, Point3D::null()};
	};
	constexpr bool anyOccupiedPoint = true;
	constexpr bool adjacent = false;
	return findPathDepthFirstWithoutMemo<decltype(destinationCondition), anyOccupiedPoint, adjacent>(start, startFacing, destinationCondition, huristicDestination, shape, m_moveType, detour, faction, Distance::max());
}
template<bool anyOccupiedPoint, bool adjacent>
FindPathResult TerrainFacade::findPathToSpaceDesignation(PathMemoBreadthFirst& memo, const SpaceDesignation designation, const FactionId& faction, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool unreserved, const Distance& maxRange) const
{
	assert(faction.exists());
	auto destinationCondition = [](const Cuboid&) -> bool { return true; };
	return findPathToSpaceDesignationAndCondition<decltype(destinationCondition), anyOccupiedPoint, adjacent>(destinationCondition, memo, designation, faction, start, startFacing, shape, detour, unreserved, maxRange);
}
template FindPathResult TerrainFacade::findPathToSpaceDesignation<false, false>(PathMemoBreadthFirst& memo, const SpaceDesignation designation, const FactionId& faction, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool unreserved, const Distance& maxRange) const;
template FindPathResult TerrainFacade::findPathToSpaceDesignation<false, true>(PathMemoBreadthFirst& memo, const SpaceDesignation designation, const FactionId& faction, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour, bool unreserved, const Distance& maxRange) const;
//TODO: this could dispatch to specific actor / item variants rather then checking actor or item twice.
FindPathResult TerrainFacade::findPathAdjacentToPolymorphicWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, bool detour) const
{
	CuboidSet targets;
	Space& space = m_area.getSpace();
	auto source = actorOrItem.getAdjacentCuboids(m_area);
	for(const Cuboid& cuboid : source)
		for(const Point3D& point : cuboid)
			if(
				space.shape_anythingCanEnterEver(point) &&
				space.shape_moveTypeCanEnter(point, m_moveType) &&
				space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(point, shape, m_moveType)
			)
				targets.maybeAdd(point);
	if(targets.empty())
		return { };
	return findPathToAnyOfWithoutMemo(start, startFacing, shape, targets, actorOrItem.getLocation(m_area), detour);
}
FindPathResult TerrainFacade::findPathToEdge(PathMemoBreadthFirst& memo, const Point3D &start, const Facing4 &startFacing, const ShapeId &shape, bool detour) const
{
	Space& space = m_area.getSpace();
	auto destinationCondition = [&](const Cuboid& cuboid) -> std::pair<bool, Point3D>
	{
		// TODO: get a point from the cuboid at the edge rather then iterating.
		if(!space.isEdge(cuboid))
			return {false, Point3D::null()};
		for(const Point3D& point : cuboid)
			return {space.isEdge(point), point};
		std::unreachable();
	};
	constexpr bool useAnyOccupiedPoint = true;
	constexpr bool adjacent = false;
	constexpr FactionId faction;
	return findPathToConditionBreadthFirst<decltype(destinationCondition), useAnyOccupiedPoint, adjacent>(destinationCondition, memo, start, startFacing, shape, detour, faction, Distance::max());
}
bool TerrainFacade::accessable(const Point3D& start, const Facing4& startFacing, const Point3D& to, const ActorIndex& actor) const
{
	Actors& actors = m_area.getActors();
	constexpr bool useAnyOccupiedPoint = false;
	constexpr bool adjacent = false;
	auto result = findPathToWithoutMemo<useAnyOccupiedPoint, adjacent>(start, startFacing, actors.getShape(actor), to);
	return !result.path.empty() || result.useCurrentPosition;
}
void AreaHasTerrainFacades::doStep()
{
	for(auto& pair : m_data)
		pair.second.doStep();
}
void AreaHasTerrainFacades::update(const Cuboid& cuboid)
{
	for(auto& pair : m_data)
		pair.second.update(cuboid);
}
void AreaHasTerrainFacades::update(const CuboidSet& cuboids)
{
	for(const Cuboid& cuboid : cuboids)
		update(cuboid);
}
void AreaHasTerrainFacades::maybeSetImpassable(const Cuboid& cuboid)
{
	for(auto& pair : m_data)
		pair.second.maybeSetImpassable(cuboid);
}
void AreaHasTerrainFacades::maybeRegisterMoveType(const MoveTypeId& moveType)
{
	//TODO: generate terrain facade in thread.
	auto found = m_data.find(moveType);
	if(found == m_data.end())
		m_data.emplace(moveType, m_area, moveType);
}
void AreaHasTerrainFacades::clearPathRequests()
{
	for(auto& pair : m_data)
		pair.second.clearPathRequests();
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
		return found->second;
}