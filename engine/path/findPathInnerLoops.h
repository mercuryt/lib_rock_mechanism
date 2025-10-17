#pragma once
#include "area/area.h"
#include "definitions/shape.h"
#include "space/space.h"
#include "numericTypes/index.h"
#include "callbackTypes.h"

class MoveType;

class PathInnerLoops
{
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool symetric>
	static FindPathResult findPathInner(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start)
	{
		// Use Point3D::null() to indicate the start.
		memo.setOpen(start);
		memo.setClosed(start, start);
		while(!memo.openEmpty())
		{
			const Point3D current = memo.next();
			//TODO: check if last seen enterable can be reused.
			const AdjacentData enterable = terrainFacade.getAdjacentData(current);
			[[maybe_unused]] const Space& space = terrainFacade.getArea().getSpace();
			const Cuboid adjacentCuboid = space.getAdjacentWithEdgeAndCornerAdjacent(current);
			const CuboidSet closed = memo.getClosedIntersecting(adjacentCuboid);
			for(const AdjacentIndex& adjacentIndex : enterable)
			{
				const Offset3D offset = current.atAdjacentIndex(adjacentIndex);
				assert(space.shape_moveTypeCanEnterFrom(Point3D::create(offset), terrainFacade.getMoveType(), current));
				assert(space.offsetBoundry().contains(offset));
				const Point3D adjacent = Point3D::create(offset);
				if(closed.contains(adjacent))
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
					return {memo.getPath(current, adjacent, start), pointWhichPassedPredicate, false};
				memo.setOpen(adjacent);
				memo.setClosed(adjacent, start);
			}
		}
		return {{}, Point3D::null(), false};
	}
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo>
	static FindPathResult findPathSetSymetric(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, bool isSymetric)
	{
		if(isSymetric)
			return findPathInner<AccessConditionT, DestinationConditionT, Memo, true>(accessCondition, destinationCondition, terrainFacade, memo, start);
		else
			return findPathInner<AccessConditionT, DestinationConditionT, Memo, false>(accessCondition, destinationCondition, terrainFacade, memo, start);
	}
	// Consumes isMultiTile, faction, and shape, all other arguments passed on to setSymetric.
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool isMultiTile>
	static FindPathResult findPathSetUnreserved(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, const FactionId& faction, const ShapeId& shape)
	{
		if(faction.exists())
		{
			const Space& space = area.getSpace();
			if(isMultiTile)
			{
				auto unreservedCondition = [&](const Point3D& point, const Facing4& facing) -> std::pair<bool, Point3D>
				{
					CuboidSet cuboids = Shape::getCuboidsOccupiedAt(shape, space, point, facing);
					if(space.isReservedAny(cuboids, faction))
						return {false, Point3D::null()};
					return destinationCondition(point, facing);
				};
				return findPathSetSymetric<AccessConditionT, decltype(unreservedCondition), Memo>(accessCondition, unreservedCondition, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
			}
			else
			{
				auto unreservedCondition = [&](const Point3D& point, const Facing4& facing) -> std::pair<bool, Point3D>
				{
					if(space.isReserved(point, faction))
						return {false, Point3D::null()};
					return destinationCondition(point, facing);
				};
				return findPathSetSymetric<AccessConditionT, decltype(unreservedCondition), Memo>(accessCondition, unreservedCondition, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
			}
		}
		else
			return findPathSetSymetric<AccessConditionT, DestinationConditionT, Memo>(accessCondition, destinationCondition, terrainFacade, memo, start, Shape::getIsRadiallySymetrical(shape));
	}
	template<AccessCondition AccessConditionT, DestinationCondition DestinationConditionT, typename Memo, bool isMultiTile>
	static FindPathResult findPathSetMaxRange(AccessConditionT accessCondition, DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, const FactionId& faction, const ShapeId& shape, const Distance& maxRange)
	{
		if(maxRange != Distance::max())
		{
			const Space& space = area.getSpace();
			auto maxRangeCondition = [&space, accessCondition, maxRange, start](const Point3D& point, const Facing4& facing) -> bool
			{
				return point.taxiDistanceTo(start) <= maxRange && accessCondition(point, facing);
			};
			return findPathSetUnreserved<decltype(maxRangeCondition), DestinationConditionT, Memo, isMultiTile>(maxRangeCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape);
		}
		else
			return findPathSetUnreserved<AccessConditionT, DestinationConditionT, Memo, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape);
	}
	template<bool isMultiTile, DestinationCondition DestinationConditionT, typename Memo>
	static FindPathResult findPathSetDetour(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const Point3D& start, const Facing4& startFacing, const FactionId& faction, const ShapeId& shape, const MoveTypeId& moveType, bool detour, const Distance& maxRange)
	{
		const Space& space = area.getSpace();
		if constexpr(isMultiTile)
		{
			if(detour)
			{
				// TODO: This is converting a SmallSet<Point3D> into a SmallSet<Point3D>, remove it when SmallSet<Point3D> is removed.
				const CuboidSet initalPoints = Shape::getCuboidsOccupiedAt(shape, space, start, startFacing);
				auto accessCondition = [&space, &initalPoints, shape, moveType](const Point3D& point, const Facing4& facing) -> bool
				{
					return space.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(point, shape, moveType, facing, initalPoints);
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
			else
			{
				auto accessCondition = [&space, shape, moveType](const Point3D& point, const Facing4& facing) -> bool
				{
					return space.shape_shapeAndMoveTypeCanEnterEverWithFacing(point, shape, moveType, facing);
				};
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
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
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
			else
			{
				auto accessCondition = [](const Point3D&, const Facing4&) -> bool { return true; };
				return findPathSetMaxRange<decltype(accessCondition), DestinationConditionT, Memo, isMultiTile>(accessCondition, destinationCondition, area, terrainFacade, memo, start, faction, shape, maxRange);
			}
		}
	}
public:
	// Consumes adjacent and anyOccupiedPoint, all other paramaters are passed on to findPathSetDetour and onward.
	// DestinationConditionT is unconstrained at this point, it might be for an occupied point rather then for a location and facing.
	template<typename Memo, bool anyOccupiedPoint, typename DestinationConditionT>
	static FindPathResult findPath(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& start, const Facing4& facing, bool detour, const FactionId& faction, const Distance maxRange)
	{
		const Space& space = area.getSpace();
		if(Shape::getIsMultiTile(shape))
		{
			if constexpr (anyOccupiedPoint)
			{
				// Multi point, any occupied.
				static_assert(PointInCuboidCondition<DestinationConditionT>);
				auto occupiedCondition = [&](const Point3D& point, const Facing4& facingAtPoint) ->std::pair<bool, Point3D>
				{
					for(const Cuboid& occupied : Shape::getCuboidsOccupiedAt(shape, space, point, facingAtPoint))
					{
						const auto [result, destination] = destinationCondition(occupied);
						if(result)
							return {true, destination};
					}
					return {false, Point3D::null()};
				};
				auto [startResult, startPointWhichPassedPredicate] = occupiedCondition(start, facing);
				if(startResult)
					return {SmallSet<Point3D>{}, startPointWhichPassedPredicate, true};
				return findPathSetDetour<true, decltype(occupiedCondition), Memo>(occupiedCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
			else
			{
				// Multi point, location point only.
				static_assert(DestinationCondition<DestinationConditionT>);
				auto [startResult, startPointWhichPassedPredicate] = destinationCondition(start, facing);
				if(startResult)
					return {SmallSet<Point3D>{}, startPointWhichPassedPredicate, true};
				return findPathSetDetour<true, DestinationConditionT, Memo>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
		}
		else
		{
			if constexpr(anyOccupiedPoint)
			{
				static_assert(PointInCuboidCondition<DestinationConditionT>);
				auto occupiedCondition = [&](const Point3D& point, const Facing4&) ->std::pair<bool, Point3D> { return destinationCondition({point, point}); };
				auto [startResult, startPointWhichPassedPredicate] = occupiedCondition(start, facing);
				if(startResult)
					return {SmallSet<Point3D>{}, startPointWhichPassedPredicate, true};
				return findPathSetDetour<false, decltype(occupiedCondition), Memo>(occupiedCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
			else
			{
				static_assert(DestinationCondition<DestinationConditionT>);
				auto [startResult, startPointWhichPassedPredicate] = destinationCondition(start, facing);
				if(startResult)
					return {SmallSet<Point3D>{}, startPointWhichPassedPredicate, true};
				return findPathSetDetour<false, DestinationConditionT, Memo>(destinationCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
			}
		}
	}
	template<typename Memo, PointInCuboidCondition DestinationConditionT, bool anyOccupiedPoint>
	static FindPathResult findPathAdjacentTo(DestinationConditionT destinationCondition, const Area& area, const TerrainFacade& terrainFacade, Memo& memo, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& start, const Facing4& facing, bool detour, const FactionId& faction, const Distance maxRange)
	{
		const Space& space = area.getSpace();
		if(Shape::getIsMultiTile(shape))
		{
			auto adjacentCondition = [shape, &space, destinationCondition](const Point3D& point, const Facing4& facingAtPoint) -> std::pair<bool, Point3D>
			{
				// TODO: filter with a boundry? Include occupied as well as adjacent?
				for(const Cuboid& adjacentCuboid : Shape::getCuboidsWhichWouldBeAdjacentAt(shape, space, point, facingAtPoint))
				{
					auto [result, pointWhichPassedPredicate] = destinationCondition(adjacentCuboid);
					if(result)
						return {true, pointWhichPassedPredicate};
				}
				return {false, Point3D::null()};
			};
			auto [startResult, startPointWhichPassedPredicate] = adjacentCondition(start, facing);
			if(startResult)
				return {{}, startPointWhichPassedPredicate, true};
			return findPathSetDetour<true, decltype(adjacentCondition), Memo>(adjacentCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
		}
		else
		{
			auto adjacentCondition = [shape, &space, destinationCondition](const Point3D& point, const Facing4&) ->std::pair<bool, Point3D>
			{
				auto [pointResult, pointWhichPassedPredicate] = destinationCondition(point.getAllAdjacentIncludingOutOfBounds());
				return {pointResult, pointWhichPassedPredicate};
			};
			auto [startResult, startPointWhichPassedPredicate] = adjacentCondition(start, facing);
			if(startResult)
				return {{}, startPointWhichPassedPredicate, true};
			return findPathSetDetour<false, decltype(adjacentCondition), Memo>(adjacentCondition, area, terrainFacade, memo, start, facing, faction, shape, moveType, detour, maxRange);
		}
	}
};