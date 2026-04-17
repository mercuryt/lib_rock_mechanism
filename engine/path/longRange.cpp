#include "longRange.h"
#include "../area/area.h"
#include "../space/space.h"
Point3D longRangePath::findCrossingPointNearestForShape(Area& area, RectangleWithFacing portal, Point3D huristic, ShapeId shape, ShortRangeMemo& memo)
{
	Cuboid nearSideOfPortal = portal.m_cuboid;
	nearSideOfPortal.shift(portal.m_facing, {1});
	SmallSet<Point3D>& openList = memo.openList;
	openList.clear();
	Point3D start = nearSideOfPortal.clamp(huristic);
	openList.insert(start);
	CuboidSet& closedList = memo.closedList;
	closedList.clear();
	closedList.add(start);
	Space& space = area.getSpace();
	while(!openList.empty())
	{
		openList.sortUnaryDescending([&](const Point3D point) { return point.distanceTo(start); });
		Point3D current = openList.back();
		openList.popBack();
		Cuboid adjacent = current.getAllAdjacentIncludingOutOfBounds().intersection(nearSideOfPortal);
		if(space.shape_canFitEverWithAnyFacing(current, shape))
		{
			Cuboid candidates = adjacent.intersection(portal.m_cuboid);
			for(const Point3D point : candidates)
				if(space.shape_canFitEver(point, shape, current.getFacingTwords(point)))
					return current;
		}
		for(const Point3D point : adjacent)
		{
			if(point == current)
				continue;
			if(closedList.contains(point))
				continue;
			closedList.add(point);
			openList.insert(point);
		}
	}
	// There must be some point where we can cross because it was already checked when finding cuboids.
	assert(false);
}
bool longRangePath::checkCuboidNotFreelyNavigable(const Cuboid cuboid, const int width, const int length, const int height)
{
	// Check if cuboid is too small to freely navigate.
	Distance narrowestDimension = std::min(cuboid.sizeX(), cuboid.sizeY());
	return narrowestDimension < width || narrowestDimension < length || cuboid.sizeZ() < height;
}
bool longRangePath::checkPortalSize(const Cuboid to, const Cuboid from, const int width, const int length, const int height)
{
	// Check if shape fits through portal.
	const Distance lowestHighZ = std::min(from.m_high.z(), to.m_high.z());
	const Distance highestLowZ = std::max(from.m_low.z(), to.m_low.z());
	const Distance differenceZ = lowestHighZ - highestLowZ;
	const Distance lowestHighX = std::min(from.m_high.x(), to.m_high.x());
	const Distance highestLowX = std::max(from.m_low.x(), to.m_low.x());
	const Distance differenceX = lowestHighX - highestLowX;
	const Distance lowestHighY = std::min(from.m_high.y(), to.m_high.y());
	const Distance highestLowY = std::max(from.m_low.y(), to.m_low.y());
	const Distance differenceY = lowestHighY - highestLowY;
	if(differenceZ == 0)
	{
		// Above or below.
		if(differenceX == 0 || differenceY == 0)
		{
			if(differenceX == 0 && differenceY == 0)
				// Corner.
				return width == 1 && height == 1;
			else if(differenceY == 0)
				return height == 1 && width <= differenceX;
			else
				return height == 1 && width <= differenceY;
		}
		return (
			(width <= differenceX && length <= differenceY) ||
			(width <= differenceY && length <= differenceX)
		);
	}
	if(differenceZ < height)
		return false;
	if(differenceX != 0)
		// East or west.
		return width <= differenceX;
	if(differenceY != 0)
		// North or south.
		return width <= differenceY;
	// edge.
	return width == 1;
}
SmallSet<Point3D> longRangePath::cuboidPathToPointPathStringPulling(Area& area, ShapeId shape, const SmallSet<Cuboid> cuboids, Point3D start, Point3D end, Point3D pointBeforeEnd, ShortRangeMemo& memo)
{
	std::vector<RectangleWithFacing> portals;
	portals.reserve(cuboids.size() - 1);
	for(int i = 1; i != cuboids.size(); ++i)
	{
		Cuboid previous = cuboids[i - 1];
		Cuboid current = cuboids[i];
		Facing6 facingFromPreviousToCurrent = previous.getFacingTwordsOtherCuboid(current);
		previous.shift(facingFromPreviousToCurrent, {1});
		portals.emplace_back(previous.intersection(current), current.getFacingTwordsOtherCuboid(previous));
	}
	SmallSet<Point3D> output = generateSegmentStringPulling(area, shape, start, pointBeforeEnd, portals.begin(), portals.end(), memo);
	output.insert(end);
	return output;
}
SmallSet<Point3D> longRangePath::generateSegmentStringPulling(Area& area, ShapeId shape, Point3D start, Point3D end, std::vector<RectangleWithFacing>::iterator startPortal, std::vector<RectangleWithFacing>::iterator endPortal, longRangePath::ShortRangeMemo& memo)
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
longRangePath::LongRangeMemo& longRangePath::checkOutMemo()
{
	std::lock_guard<std::mutex> lock(memoMutex);
	int end = memos.size();
	for(int i = 0; i != end; ++i)
		if(!memoCheckedOut[i])
		{
			memoCheckedOut[i] = true;
			return *memos[i];
		}
	// All Memos are checked out, create new.
	memos.emplace_back(std::make_unique<LongRangeMemo>());
	memoCheckedOut.emplace_back(true);
	return *memos.back();
}
void longRangePath::returnMemo(LongRangeMemo& memo)
{
	std::lock_guard<std::mutex> lock(memoMutex);
	int end = memos.size();
	for(int i = 0; i != end; ++i)
		if(memos[i].get() == &memo)
		{
			memoCheckedOut[i] = false;
			return;
		}
	assert(false);
}