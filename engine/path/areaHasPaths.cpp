#include "areaHasPaths.hpp"
#include "longRange.h"
#include "pathRequest.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../definitions/moveType.h"
AreaHasPathsForMoveType::AreaHasPathsForMoveType(Area& area, const MoveTypeId moveType) :
	m_enterable(area.getSpace()),
	m_moveType(moveType)
{
	update(area, area.getSpace().boundry());
}
void AreaHasPathsForMoveType::doStep(Area& area)
{
	Actors& actors = area.getActors();
	actors.move_clearAllPathRequests();
	for(std::unique_ptr<PathRequest>& pathRequestPtr : m_pathRequests)
	{
		PathRequest& pathRequest = *pathRequestPtr.get();
		auto [path, target] = pathRequest.readStep(area, *this);
		pathRequest.path = path;
		pathRequest.target = target;
	}
	auto requests = std::move(m_pathRequests);
	m_pathRequests.clear();
	for(std::unique_ptr<PathRequest>& pathRequestPtr : requests)
	{
		PathRequest& pathRequest = *pathRequestPtr.get();
		bool useCurrentLocation = pathRequest.target.exists() && pathRequest.path.empty();
		pathRequest.writeStep(area, useCurrentLocation);
	}
}
void AreaHasPathsForMoveType::update(Area& area, const Cuboid cuboid)
{
	Space& space = area.getSpace();
	// Update enterable cuboids.
	m_enterable.maybeRemove(cuboid);
	CuboidSet enterable = space.move_queryPathable(cuboid, m_moveType);
	for(const Cuboid enterableCuboid : enterable)
		for(const Cuboid partitionedCuboid : space.move_splitCuboidByPartitions(enterableCuboid))
			m_enterable.insert(partitionedCuboid);
	m_enterable.prepare();
	// Update vertical clearance below.
	CuboidSet levelBelow = CuboidSet::create(cuboid.getFaceBelow());
	Distance z = cuboid.m_low.z();
	const Distance cuboidBottom = cuboid.m_low.z();
	SmallSet<Cuboid> toUpdate;
	// TODO:(optimization) This doesn't have to be a linear search.
	while(z != 0)
	{
		levelBelow.shift({0, 0, -1}, {1});
		--z;
		// Remove all solid or features which block enterance from all directions.
		space.move_removeUnenterableFrom(levelBelow);
		if(levelBelow.empty())
			break;
		m_enterable.queryForEachWithCuboids(levelBelow, [&toUpdate, cuboidBottom](Cuboid enterableCuboid, EnterableData enterableData){
			Distance clearanceLevel = enterableCuboid.m_high.z() + enterableData.verticalClearance;
			if(clearanceLevel >= cuboidBottom)
				// The vertical clearance level is high enough to be effected by changes in cuboid.
				toUpdate.maybeInsert(enterableCuboid);
		});
		// Remove features which block entrance from below from query set.
		space.pointFeature_queryForEachWithCuboids(levelBelow.boundry(), [&levelBelow](Cuboid featureCuboid, PointFeature feature){
			if(feature.blocksVerticalTravel())
				levelBelow.maybeRemove(featureCuboid);
		});
		if(levelBelow.empty())
			break;
	}
	m_enterable.updateVerticalClearance(toUpdate);
}
void AreaHasPathsForMoveType::recordPathRequest(std::unique_ptr<PathRequest> pathRequest)
{
	m_pathRequests.push_back(std::move(pathRequest));
}
void AreaHasPathsForMoveType::cancelPathRequest(PathRequest& pathRequest)
{
	auto found = std::ranges::find(m_pathRequests, &pathRequest, &std::unique_ptr<PathRequest>::get);
	if(m_pathRequests.begin() + m_pathRequests.size() - 1 != found)
		// Not last.
		(*found) = std::move(m_pathRequests.back());
	m_pathRequests.pop_back();
}
PathResult AreaHasPathsForMoveType::pathTo(const PathParamaters& params) const
{
	const Point3D destination = params.huristicDestination;
	auto longRangeCondition = [destination](const Cuboid cuboid) -> bool { return cuboid.contains(destination); };
	if(params.anyOccupiedPoint)
	{
		assert(!params.adjacent);
		auto shortRangeCondition = [destination](const Cuboid cuboid) -> Point3D { return cuboid.contains(destination) ? destination : Point3D::null(); };
		return longRangePath::getPathAnyOccupied(m_enterable, longRangeCondition, shortRangeCondition, params);
	}
	if(params.adjacent)
	{
		auto shortRangeCondition = [destination](const Cuboid cuboid) -> Point3D { return cuboid.contains(destination) ? destination : Point3D::null(); };
		return longRangePath::getPathAdjacent(m_enterable, longRangeCondition, shortRangeCondition, params);
	}
	else
	{
		auto shortRangeCondition = [destination](const Point3D point, const Facing4) -> Point3D { return point == destination ? point : Point3D::null(); };
		return longRangePath::getPath(m_enterable, longRangeCondition, shortRangeCondition, params);
	}
}
PathResult AreaHasPathsForMoveType::pathToEdge(const PathParamaters& params) const
{
	const Space& space = params.area.getSpace();
	Cuboid boundry = space.boundry();
	auto longRangeCondition = [boundry](const Cuboid cuboid) -> bool { return cuboid.isTouchingFaceFromInside(boundry); };
	auto shortRangeCondition = [&space, &params, boundry](const Point3D point, const Facing4 facing) -> Point3D
	{
		Cuboid shapeBoundry = Shape::getBoundryAtWithFacing(params.shape, space, point, facing);
		return shapeBoundry.isTouchingFaceFromInside(boundry) ? point : Point3D::null();
	};
	return longRangePath::getPath(m_enterable, longRangeCondition, shortRangeCondition, params);
}
PathResult AreaHasPathsForMoveType::pathToDesignation(SpaceDesignation designation, const PathParamaters& params) const
{
	const Space& space = params.area.getSpace();
	auto longRangeCondition = [&space, &params, designation](const Cuboid cuboid) -> bool
	{
		return space.designation_has(cuboid, params.faction, designation);
	};
	if(params.adjacent || params.anyOccupiedPoint)
	{
		auto shortRangeCondition = [&space, &params, designation](const Cuboid cuboid) -> Point3D
		{
			return space.designation_hasPoint(cuboid, params.faction, designation);
		};
		if(params.adjacent)
			return pathToCondition<true, false>(longRangeCondition, shortRangeCondition, params);
		assert(params.anyOccupiedPoint);
		return pathToCondition<false, true>(longRangeCondition, shortRangeCondition, params);
	}
	else
	{
		auto shortRangeCondition = [&space, &params, designation](const Point3D point, const Facing4) -> Point3D
		{
			return space.designation_has(point, params.faction, designation) ? point : Point3D::null();
		};
		return pathToCondition<false, false>(longRangeCondition, shortRangeCondition, params);
	}
}
bool AreaHasPathsForMoveType::accessable(const PathParamaters& params) const
{
	assert(!params.returnPath);
	return pathTo(params).found();
}
void AreaHasPaths::doStep(Area& area)
{
	for(AreaHasPathsForMoveType& hasPathsForMoveType : m_data)
		hasPathsForMoveType.doStep(area);
}
void AreaHasPaths::registerMoveType(Area& area, const MoveTypeId moveType) { m_data.emplace_back(area, moveType); }
void AreaHasPaths::maybeRegisterMoveType(Area& area, const MoveTypeId moveType)
{
	if(std::ranges::find(m_data, moveType, &AreaHasPathsForMoveType::m_moveType) == m_data.end())
		registerMoveType(area, moveType);
}
AreaHasPathsForMoveType& AreaHasPaths::get(const MoveTypeId moveType)
{
	auto found = std::ranges::find(m_data, moveType, &AreaHasPathsForMoveType::m_moveType);
	assert(found != m_data.end());
	return *found;
}
void AreaHasPaths::clearPathRequests()
{
	for(AreaHasPathsForMoveType& hasPaths : m_data)
		hasPaths.m_pathRequests.clear();
}
void AreaHasPaths::update(Area& area, const Cuboid cuboid)
{
	for(AreaHasPathsForMoveType& forMoveType : m_data)
		forMoveType.update(area, cuboid);
}
void AreaHasPaths::maybeSetImpassable(const Cuboid cuboid)
{
	for(AreaHasPathsForMoveType& forMoveType : m_data)
		forMoveType.m_enterable.maybeRemove(cuboid);
}