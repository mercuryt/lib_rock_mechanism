#pragma once
#include "area/area.h"
#include "definitions/shape.h"
#include "space/space.h"
#include "numericTypes/index.h"
#include "callbackTypes.h"

class MoveType;

class PathInnerLoops
{
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool symetric, bool adjacentSingleTile>
	static FindPathResult findPathInner(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start)
	{
		const Space& space =  area.getSpace();
		// Use Point3D::max to indicate the start.
		memo.setOpen(start, area);
		memo.setClosed(start, Point3D::max());
		while(!memo.openEmpty())
		{
			Point3D current = memo.next();
			const auto allAdjacent = current.getAllAdjacentIncludingOutOfBounds();
			for(AdjacentIndex adjacentCount = AdjacentIndex::create(0); adjacentCount < maxAdjacent; ++adjacentCount)
			{
				Point3D adjacent = allAdjacent[adjacentCount.get()];
				if(!space.getAll().contains(adjacent))
					// No point here, edge of area.
					continue;
				if constexpr (adjacentSingleTile)
				{
					Facing4 facing;
					if constexpr(!symetric)
						facing = current.getFacingTwords(adjacent);
					else
						facing = Facing4::North;
					if(memo.isClosed(adjacent))
						continue;
					//[[maybe_unused]] Point3D coordinates = adjacent;
					//[[maybe_unused]] bool stopHere = coordinates.y() == 7 && coordinates.x() == 2 && coordinates.z() == 1;
					auto [result, pointWhichPassedPredicate] = destinationCondition(adjacent, facing);
					if(result)
					{
						const SmallSet<Point3D>& path = memo.getPath(current, Point3D::null());
						return {path, pointWhichPassedPredicate, path.empty()};
					}
					if(terrainFacade.canEnterFrom(current, adjacentCount) && accessCondition(adjacent, facing))
					{
						memo.setOpen(adjacent, area);
						memo.setClosed(adjacent, current);
					}
				}
				else if(terrainFacade.canEnterFrom(current, adjacentCount))
				{
					assert(adjacent.exists());
					if(memo.isClosed(adjacent))
						continue;
					Facing4 facing;
					if constexpr(!symetric)
						facing = current.getFacingTwords(adjacent);
					else
						facing = Facing4::North;
					if(!accessCondition(adjacent, facing))
						continue;
					auto [result, pointWhichPassedPredicate] = destinationCondition(adjacent, facing);
					if(result)
					{
						return {memo.getPath(current, adjacent), pointWhichPassedPredicate, false};
					}
					memo.setOpen(adjacent, area);
					memo.setClosed(adjacent, current);
				}
			}
		}
		return {{}, Point3D::null(), false};
	}
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool adjacentSingleTile>
	static FindPathResult findPathSetSymetric(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, bool isSymetric)
	{
		if(isSymetric)
			return findPathInner<AccessConditionT, DestinationConditionT, Memo, true, adjacentSingleTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start);
		else
			return findPathInner<AccessConditionT, DestinationConditionT, Memo, false, adjacentSingleTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start);
	}
	// Consumes isMultiTile, faction, and shape, all other arguments passed on to setSymetric.
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool adjacentSingleTile, bool isMultiTile>
	static FindPathResult findPathSetUnreserved(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, const FactionId& faction, const ShapeId& shape)
	{
		if(faction.exists())
		{
			const Space& space =  area.getSpace();
			if(isMultiTile)
			{
				auto unreservedCondition = [&](const Point3D& point, const Facing4& facing) -> std::pair<bool, Point3D>
				{
					for(const Point3D& point : Shape::getPointsOccupiedAt(shape, space, point, facing))
						if(space.isReserved(point, faction))
							return {false, point};
					return destinationCondition(point, facing);
				};
				return findPathSetSymetric<AccessConditionT, decltype(unreservedCondition), Memo, adjacentSingleTile>(accessCondition, unreservedCondition, area, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
			}
			else
			{
				auto unreservedCondition = [&](const Point3D& point, const Facing4& facing) -> std::pair<bool, Point3D>
				{
					if(space.isReserved(point, faction))
						return {false, point};
					return destinationCondition(point, facing);
				};
				return findPathSetSymetric<AccessConditionT, decltype(unreservedCondition), Memo, adjacentSingleTile>(accessCondition, unreservedCondition, area, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
			}
		}
		else
			return findPathSetSymetric<AccessConditionT, DestinationConditionT, Memo, adjacentSingleTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
	}
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool adjacentSingleTile, bool isMultiTile>
	static FindPathResult findPathSetMaxRange(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, const FactionId& faction, const ShapeId& shape, const Distance& maxRange)
	{
		if(maxRange != Distance::max())
		{
			const Space& space =  area.getSpace();
			auto maxRangeCondition = [&space, accessCondition, maxRange, start](const Point3D& point, const Facing4& facing) -> bool
			{
				return point.taxiDistanceTo(start) <= maxRange && accessCondition(point, facing);
			};
			return findPathSetUnreserved<decltype(maxRangeCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(maxRangeCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape);
		}
		else
			return findPathSetUnreserved<AccessConditionT, DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape);
	}
	template<bool adjacentSingleTile, bool isMultiTile, DestinationCondition DestinationConditionT, typename Memo>
	static FindPathResult findPathSetDetour(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, const Facing4& startFacing, const FactionId& faction, const ShapeId& shape, const MoveTypeId& moveType, bool detour, const Distance& maxRange)
	{
		const Space& space =  area.getSpace();
		if constexpr(isMultiTile)
		{
			if(detour)
			{
				// TODO: This is converting a SmallSet<Point3D> into a SmallSet<Point3D>, remove it when SmallSet<Point3D> is removed.
				const OccupiedSpaceForHasShape initalPoints = OccupiedSpaceForHasShape::create(Shape::getPointsOccupiedAt(shape, space, start, startFacing));
				auto accessCondition = [&space, &initalPoints, shape, moveType](const Point3D& point, const Facing4& facing) -> bool
				{
					return space.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(point, shape, moveType, facing, initalPoints);
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
			else
			{
				auto accessCondition = [&space, shape, moveType](const Point3D& point, const Facing4& facing) -> bool
				{
					return space.shape_shapeAndMoveTypeCanEnterEverWithFacing(point, shape, moveType, facing);
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
		}
		else
		{
			if(detour)
			{
				CollisionVolume volume = Shape::getCollisionVolumeAtLocation(shape);
				auto accessCondition = [start, &space, maxRange, volume](const Point3D& point, const Facing4&) -> bool
				{
					if (maxRange != Distance::max())
						if(start.taxiDistanceTo(point) > maxRange)
							return false;
					return point == start || space.shape_getDynamicVolume(point) + volume <= Config::maxPointVolume;
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
			else
			{
				auto accessCondition = [](const Point3D&, const Facing4&) -> bool { return true; };
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, adjacentSingleTile, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
		}
	}
public:
	// Consumes adjacent and anyOccupiedPoint, all other paramaters are passed on to findPathSetDetour and onward.
	template<typename Memo, bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	static FindPathResult findPath(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& start, const Facing4& facing, bool detour, bool adjacent, const FactionId& faction, const Distance maxRange)
	{
		auto [result, pointWhichPassedPredicate] = destinationCondition(start, facing);
		if(result)
			return {{}, pointWhichPassedPredicate, true};
		const Space& space =  area.getSpace();
		if(Shape::getIsMultiTile(shape))
		{
			if(adjacent)
			{
				auto adjacentCondition = [shape, &space, destinationCondition](const Point3D& point, const Facing4& facing) ->std::pair<bool, Point3D>
				{
					for(const Point3D& adjacent : Shape::getPointsWhichWouldBeAdjacentAt(shape, space, point, facing))
						if(destinationCondition(adjacent, Facing4::Null).first)
							return {true, adjacent};
					return {false, point};
				};
				return findPathSetDetour<false, true, decltype(adjacentCondition), decltype(memo)>(adjacentCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
			else
			{
				if constexpr (anyOccupiedPoint)
				{
					auto occupiedCondition = [shape, &space, destinationCondition](const Point3D& point, const Facing4& facing) ->std::pair<bool, Point3D>
					{
						for(const Point3D& occupied : Shape::getPointsOccupiedAt(shape, space, point, facing))
							if(destinationCondition(occupied, Facing4::Null).first)
								return {true, occupied};
						return {false, point};
					};
					return findPathSetDetour<false, true, decltype(occupiedCondition), decltype(memo)>(occupiedCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
				}
				return findPathSetDetour<false, true, decltype(destinationCondition), decltype(memo)>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
		}
		else
		{
			if(adjacent)
				// Don't wrap destination condition with an adjacentCondition here: use the singleTileAdjacent optimization instead.
				return findPathSetDetour<true, false, decltype(destinationCondition), decltype(memo)>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			else
				return findPathSetDetour<false, false, decltype(destinationCondition), decltype(memo)>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
		}
	}
};