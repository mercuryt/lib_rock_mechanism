#include "longRange.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../threads.h"
#include <omp.h>
void longRangePath::init()
{
	assert(memos.empty());
	memos.resize(threads::max);
}
Point3D longRangePath::findCrossingPointNearestForShape(const Area& area, RectangleWithFacing portal, Point3D huristic, ShapeId shape, ShortRangeMemo& memo)
{
	Cuboid nearSideOfPortal = portal.m_cuboid;
	nearSideOfPortal.shift(portal.m_facing, {1});
	std::vector<Point3D>& openList = memo.openList;
	openList.clear();
	Point3D start = nearSideOfPortal.clamp(huristic);
	openList.push_back(start);
	CuboidSet& closedList = memo.closedList;
	closedList.clear();
	closedList.add(start);
	const Space& space = area.getSpace();
	while(!openList.empty())
	{
		auto iter = std::ranges::min_element(openList, {}, [&](const Point3D point) { return point.distanceTo(start); });
		Point3D current = (*iter);
		(*iter) = openList.back();
		openList.pop_back();
		Cuboid adjacent = current.getAllAdjacentIncludingOutOfBounds().intersection(nearSideOfPortal);
		if(space.shape_canFitEverWithAnyFacing(current, shape))
		{
			Cuboid candidates = adjacent.intersection(portal.m_cuboid);
			for(const Point3D point : candidates)
				if(space.shape_canFitEver(point, shape, current.getFacingTwords(point)))
					return current;
		}
		CuboidSet filteredAdjacent = CuboidSet::create(adjacent);
		filteredAdjacent.maybeRemoveAll(closedList);
		closedList.maybeAddAll(filteredAdjacent);
		for(const Cuboid cuboid : filteredAdjacent)
			for(const Point3D point : cuboid)
				openList.push_back(point);
	}
	// There must be some point where we can cross because it was already checked when finding cuboids.
	assert(false);
}
bool longRangePath::checkCuboidNotFreelyNavigable(const Cuboid cuboid, const PathParamaters& params)
{
	// Check if cuboid is too small to freely navigate.
	Distance narrowestDimension = std::min(cuboid.sizeX(), cuboid.sizeY());
	return narrowestDimension < params.width || narrowestDimension < params.length || cuboid.sizeZ() < params.height;
}
bool longRangePath::checkPortalSize(const Cuboid to, const Cuboid from, const PathParamaters& params)
{
	assert(to.isTouchingFace(from));
	// Check if shape fits through portal.
	Distance overlapX;
	Distance overlapY;
	Distance overlapZ;
	const Distance lowestHighZ = std::min(from.m_high.z(), to.m_high.z());
	const Distance highestLowZ = std::max(from.m_low.z(), to.m_low.z());
	if(highestLowZ <= lowestHighZ)
		overlapZ = lowestHighZ - highestLowZ;
	const Distance lowestHighX = std::min(from.m_high.x(), to.m_high.x());
	const Distance highestLowX = std::max(from.m_low.x(), to.m_low.x());
	if(highestLowX <= lowestHighX)
		overlapX = lowestHighX - highestLowX;
	const Distance lowestHighY = std::min(from.m_high.y(), to.m_high.y());
	const Distance highestLowY = std::max(from.m_low.y(), to.m_low.y());
	if(highestLowY <= lowestHighY)
		overlapY = lowestHighY - highestLowY;
	if(overlapZ.empty())
	{
		Cuboid above = Cuboid::create(Point3D(lowestHighX, lowestHighY, lowestHighZ), Point3D(highestLowX, highestLowY, lowestHighZ));
		const Space& space = params.area.getSpace();
		if(!space.pointFeature_canEnterFromBelowAll(above))
			// There is a horizontal partition blocking some part of the portal, return false and do the slow check.
				return false;
		// Above or below.
		return (
			(params.width <= overlapX && params.length <= overlapY) ||
			(params.width <= overlapY && params.length <= overlapX)
		);
	}
	if(overlapZ < params.height)
		return false;
	if(overlapX.exists())
		// East or west.
		return params.width <= overlapX;
	assert(overlapY.exists());
	// North or south.
	return params.width <= overlapY;
}
SmallSet<Point3D> longRangePath::cuboidPathToPointPathStringPulling(const Area& area, ShapeId shape, const std::vector<Cuboid> cuboids, Point3D start, Point3D end, Point3D pointBeforeEnd, ShortRangeMemo& memo)
{
	std::vector<RectangleWithFacing> portals;
	portals.reserve(cuboids.size() - 1);
	const int cuboidsCount = cuboids.size();
	for(int i = 1; i != cuboidsCount; ++i)
	{
		Cuboid previous = cuboids[i - 1];
		Cuboid current = cuboids[i];
		Facing6 facingFromPreviousToCurrent = previous.getFacing6TwordsOtherCuboid(current);
		previous.shift(facingFromPreviousToCurrent, {1});
		portals.emplace_back(previous.intersection(current), current.getFacing6TwordsOtherCuboid(previous));
	}
	SmallSet<Point3D> output = generateSegmentStringPulling(area, shape, start, pointBeforeEnd, portals.begin(), portals.end(), memo);
	output.insert(end);
	return output;
}
SmallSet<Point3D> longRangePath::generateSegmentStringPulling(const Area& area, ShapeId shape, Point3D start, Point3D end, std::vector<RectangleWithFacing>::iterator startPortal, std::vector<RectangleWithFacing>::iterator endPortal, longRangePath::ShortRangeMemo& memo)
{
	LinePath linePath = LinePath::create(start, end);
	std::vector<RectangleWithFacing>::iterator portalWhichCorrespondsToLastWaypoint = startPortal;
	Point3D lastWaypoint = start;
	for(auto portal = startPortal; portal != endPortal; ++portal)
	{
		ParamaterizedLine lastLine = linePath.last();
		Plane planeFromRectangle = portal->getPlane();
		Point3D intersection = planeFromRectangle.intersection(lastLine);
		Point3D clamped = findCrossingPointNearestForShape(area, *portal, intersection, shape, memo);
		if(clamped != intersection)
		{
			// Intersection was clamped, insert as waypoint.
			// Recurse newly created segment prior to waypoint.
			SmallSet<Point3D> waypoints = generateSegmentStringPulling(area, shape, lastWaypoint, clamped, portalWhichCorrespondsToLastWaypoint, portal, memo);
			// First waypoint of sequence is already in the path, omit it from insert.
			// Last waypoint of sequnce is 'clamped', insert along with recursively generated waypoints, if any.
			auto iteratorToLastPoint = linePath.m_points.begin() + linePath.m_points.size() - 1;
			linePath.m_points.insert(iteratorToLastPoint, waypoints.m_data.begin() + 1, waypoints.m_data.end());
			// Next segment starts from clamped, starting after this portal.
			portalWhichCorrespondsToLastWaypoint = std::next(portal);
			lastWaypoint = clamped;
		}
	}
	return linePath.toPoints();
}