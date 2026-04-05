#pragma once
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/linePath.h"
#include "../geometry/rectangle.h"
#include "../space/space.h"
#include "../area/area.h"
namespace longRangePath
{
	struct OpenListData
	{
		Cuboid current;
		Cuboid previous;
		int cost;
		bool shortRange;
	};
	struct ShortRangeMemo
	{
		std::vector<Point3D> openList;
		SmallMap<Point3D, Point3D> closedListToFrom;
	};
	struct LongRangeMemo
	{
		ShortRangeMemo shortRangeMemo;
		std::vector<OpenListData> openList;
		SmallMap<Cuboid, Cuboid> closedListToFrom;

	};
	inline std::vector<std::unique_ptr<LongRangeMemo>> memos;
	inline std::vector<bool> memoCheckedOut;
	inline std::mutex memoMutex;
	LongRangeMemo checkOutMemo()
	{
		std::lock_guard(memoMutex);
		int end = memos.size();
		for(int i = 0; i != end; ++i)
			if(!memoCheckedOut.at(i))
			{
				memoCheckedOut.assign(i, true);
				return *memos[i];
			}
		// All Memos are checked out, create new.
		memos.emplace_back(std::make_unique<LongRangeMemo>());
		memoCheckedOut.emplace_back(true);
		return *memos.back();
	}
	void returnMemo(LongRangeMemo& memo)
	{
		std::lock_guard(memoMutex);
		int end = memos.size();
		for(int i = 0; i != end; ++i)
			if(memos[i].get() == &memo)
			{
				memoCheckedOut.assign(i, false);
				return;
			}
		assert(false);
	}
	template<bool depthFirst, bool singleTile>
	std::pair<SmallSet<Cuboid>, Point3D> getAdjacentPathToConditionWithShape(auto& rtree, const Point3D start, const ShapeId shape, auto&& condition, LongRangeMemo& memo, const Point3D huristicDestination = {})
	{
		// TODO: if cuboid is narrower then width in x or y prove a path for shape from previous cuboid to next cuboid before adding to openList.
		if constexpr(depthFirst)
			assert(huristicDestination.exists());
		const Distance length = Shape::getLength(shape);
		const Distance width = Shape::getWidth(shape);
		const Distance height = Shape::getHeight(shape);
		std::vector<OpenListData>& openList = memo.openList;
		openList.clear();
		const Cuboid startCuboid = rtree.queryGetOneCuboid(start);
		openList.emplace_back(0, startCuboid, Cuboid::null());
		SmallMap<Cuboid, Cuboid>& closedListToFrom = memo.closedListToFrom;
		closedListToFrom.clear();
		closedListToFrom.insert(startCuboid, Cuboid::null());
		Cuboid previous;
		while(!openList.empty())
		{
			std::ranges::sort(openList, std::greater{}, &OpenListData::cost);
			const Cuboid previous = openList.back().previous;
			const Cuboid current = openList.back().current;
			const int cost = openList.back().cost;
			const bool shortRange = openList.back().shortRange;
			openList.pop_back();
			auto checkPortalSize = [&](const Cuboid& other) -> bool
			{
				const Distance lowestHighZ = std::min(current.m_high.z(), other.m_high.z());
				const Distance highestLowZ = std::max(current.m_low.z(), other.m_low.z());
				const Distance differenceZ = highestLowZ - lowestHighZ;
				const Distance lowestHighX = std::min(current.m_high.x(), other.m_high.x());
				const Distance highestLowX = std::max(current.m_low.x(), other.m_low.x());
				const Distance differenceX = highestLowX - lowestHighX;
				const Distance lowestHighY = std::min(current.m_high.y(), other.m_high.y());
				const Distance highestLowY = std::max(current.m_low.y(), other.m_low.y());
				const Distance differenceY = highestLowY - lowestHighY;
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
			};
			auto withEachCuboid = [&](const Cuboid cuboid) -> std::pair<SmallSet<Cuboid>, Point3D> {
				if(closedListToFrom.contains(cuboid))
					return {{}, Point3D::null()};
				bool shortRangeEnabled = false;
				Point3D destination = condition(cuboid);
				if(destination.exists())
				{
					//Destination found, build path.
					SmallSet<Cuboid> path;
					Cuboid currentHistory = cuboid;
					while(currentHistory != startCuboid)
					{
						path.insert(currentHistory);
						currentHistory = m_closedListToFrom[current];
					}
					return {path, destination};
				}
				if constexpr(!singleTile)
				{
					if(!checkPortalSize(cuboid))
					{
						// Shape does not fit through gap between cuboids but might still be able to pass due to a hypothetical third cuboid.
						// Find a point in current which is adjacent to cuboid where shape can fit facing cuboid and can step forward.
						Facing6 facingTwordsCurrent = cuboid.getFacingTwordsOtherCuboid(current);
						Cuboid shifted = cuboid.shifted(facingTwordsCurrent, {1});
						Cuboid from = shifted.intersection(current);
						bool found = false;
						for(const Point3D point : from)
							if(space.shape_canFitEverWithAnyFacing(point, shape));
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
							continue;
						shortRangeEnabled = true;
					}
					Distance narrowestDimension = std::min(cuboid.sizeX(), cuboid.sizeY());
					bool canNotTurn = narrowestDimension <= width;
					if(canNotTurn)
						shortRangeEnabled = true;
				}
				closedListToFrom.insert(cuboid, current);
				int newCost = cost;
				if(previous.exists())
					newCost += previous.distanceTo(cuboid);
				else
					newCost += start.distanceTo(cuboid);
				if constexpr(depthFirst)
					newCost += center.distanceTo(huristicDestination);
				openList.emplace_back(cuboid, current, newCost, shortRangeEnabled);
				return {{}, Point3D::null()};
			};
			if(shortRange)
			{
				for(const Cuboid accessableCuboid : longRangePath::getAccessable(rtree, current, memo.shortRangeMemo))
					withEachCuboid(accessableCuboid);
			}
			else
				rtree.queryForEachCuboids(current.inflated({1}), [&](const Cuboid adjacentCuboid){ withEachCuboid(adjacentCuboid); });
		}
		// No route found.
		return {{}, Point3D::null()};
	}
	SmallSet<Cuboid> getAccessable(Area& area, const auto& rtree, const Cuboid current, const Cuboid previous, const ShapeId shape, ShortRangeMemo& memo)
	{
		SmallSet<Cuboid> candidates = rtree.queryGetAllLeaves(current.inflated({1}));
		SmallSet<Cuboid> output;
		candidates.erase(current);
		std::vector<Point3D>& openList = memo.openList;
		openList.clear();
		openList.reserve(current.volume() / 2);
		SmallMap<Point3D, Point3D>& closedListToFrom = memo.closedListToFrom;
		closedListToFrom.clear();
		closedListToFrom.reserve(current.volume());
		Facing6 facingFromCuboidToPrevious = current.getFacingTwordsOtherCuboid(previous);
		Cuboid shiftedCuboid = current.shift(facingFromCuboidToPrevious, {1});
		Cuboid overlap = shiftedCuboid.intersection(previous);
		Space& space = area.getSpace();
		for(const Point3D point : overlap)
			if(space.shape_canFitEver(point, shape, previous.getFacingTwordsOtherCuboid(current)))
			{
				openList.insert(point);
				closedList.emplace_back(point, Point3D::null());
			}
		while(!openList.empty())
		{
			Point3D currentPoint = openList.back();
			openList.pop_back();
			if(!current.contains(currentPoint))
			{
				// The point is outside the bounds of the cuboid being searched.
				for(const Cuboid candidate : candidates)
					if(candidate.contains(currentPoint))
					{
						output.insert(candidate);
						if(output.size() == candidates.size())
							return output;
						break;
					}
			}
			else
			{
				Cuboid adjacentCuboid = space.getAdjacentWithEdgeAndCornerAdjacent(currentPoint);
				for(Point3D adjacentPoint : adjacentCuboid)
				{
					if(closedListToFrom.contains(adjacentPoint))
						continue;
					if(!shape.canFitEver(adjacentPoint, shape, currentPoint))
						continue;
					closedListToFrom.insert(adjacentPoint, currentPoint);
					openList.insert(adjacentPoint);
				}
			}
		}
		return output;
	}
	SmallSet<Point3D> cuboidPathToPointPath(Area& area, ShapeId shape, const SmallSet<Cuboid> cuboids, Point3D start, Point3D end)
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
		return generateSegment(area, shape, start, end, portals.begin(), portals.end());
	}
	SmallSet<Point3D> generateSegment(Area& area, ShapeId shape, Point3D start, Point3D end, std::vector<RectangleWithFacing>::iterator startPortal, std::vector<RectangleWithFacing>::iterator endPortal)
	{
		std::vector<RectangleWithFacing>::iterator portalWhichCorrespondsToLastWaypoint = startPortal;
		LinePath linePath = LinePath::create(start, end);
		for(auto portal = startPortal; portal != endPortal; ++portal)
		{
			SmallSet<Point3D> output;
			ParamaterizedLine lastLine = linePath.last();
			Plane planeFromRectangle = portal->getPlane();
			Point3D intersection = planeFromRectangle.intersection(lastLine);
			Point3D clamped = findCrossingPointNearestForShape(area, *portal, intersection, shape);
			if(clamped != intersection)
			{
				// Intersection was clamped, insert as waypoint.
				// Recurse newly created segment prior to waypoint.
				std::vector<RectangleWithFacing>::iterator startOfPortalSubSegment;
				Point3D lastWaypoint = linePath.m_points[linePath.m_points.size() - 1];
				SmallSet<Point3D> waypoints = generateSegment(area, shape, lastWaypoint, clamped, portalWhichCorrespondsToLastWaypoint, portal);
				// First waypoint of sequence is already in the path, omit it from insert.
				// Last waypoint of sequnce is 'clamped', insert along with recursively generated waypoints, if any.
				linePath.m_points.insert(linePath.m_points.end(), waypoints.m_data.begin() + 1, waypoints.m_data.end());
				portalWhichCorrespondsToLastWaypoint = portal;
			}
		}
		return linePath.toPoints();
	}
	Point3D findCrossingPointNearestForShape(Area& area, RectangleWithFacing portal, Point3D huristic, ShapeId shape)
	{
		Cuboid nearSideOfPortal = portal.m_cuboid;
		nearSideOfPortal.shift(portal.m_facing, {1});
		SmallSet<Point3D> openList;
		Point3D start = nearSideOfPortal.clamp(huristic);
		openList.insert(start);
		SmallSet<Point3D> closedList;
		closedList.insert(start);
		Space& space = area.getSpace();
		while(!openList.empty())
		{
			openList.sort([&](const Point3D point) { return point.distanceTo(start); });
			Point3D current = openList.back();
			openList.popBack();
			Cuboid adjacent = current.getAllAdjacentIncludingOutOfBounds();
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
				closedList.insert(point);
				openList.insert(point);
			}
		}
	}
}