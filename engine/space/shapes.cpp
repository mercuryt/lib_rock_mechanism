#include "space.h"
#include "../definitions/shape.h"
#include "../definitions/itemType.h"
#include "../definitions/moveType.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
bool Space::shape_anythingCanEnterEver(const Point3D& point) const
{
	if(isDynamic(point))
		return true;
	if(solid_is(point))
		return false;
	return !pointFeature_blocksEntrance(point);
}
bool Space::shape_anythingCanEnterCurrently(const Point3D& point) const
{
	// TODO: cache this in a bitset.
	return !(isDynamic(point) && (solid_is(point) || pointFeature_blocksEntrance(point)));
}
bool Space::shape_canFitEverOrCurrentlyDynamic(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const
{
	const Cuboid spaceBoundry = boundry();
	const Offset3D pointOffset = point.toOffset();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		if(
			!shape_anythingCanEnterEver(otherPoint) ||
			(
				(
					!shape_anythingCanEnterCurrently(otherPoint) ||
					(m_dynamicVolume.queryGetOneOr(otherPoint, CollisionVolume::create(0)) + pair.volume > Config::maxPointVolume)
				) &&
				!occupied.contains(otherPoint)
			)
		)
			return false;
	}
	return true;
}
bool Space::shape_canFitEverOrCurrentlyStatic(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const
{
	const Cuboid spaceBoundry = boundry();
	const Offset3D pointOffset = point.toOffset();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		if(
			!shape_anythingCanEnterEver(otherPoint) ||
			!shape_anythingCanEnterCurrently(otherPoint) ||
			(
				(m_staticVolume.queryGetOne(otherPoint) + pair.volume > Config::maxPointVolume ) &&
				!occupied.contains(otherPoint)
			)
		)
			return false;
	}
	return true;
}
bool Space::shape_canFitEver(const Point3D& point, const ShapeId& shape, const Facing4& facing) const
{
	assert(shape_anythingCanEnterEver(point));
	const Cuboid spaceBoundry = boundry();
	const Offset3D pointOffset = point.toOffset();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		if(!shape_anythingCanEnterEver(otherPoint))
			return false;
	}
	return true;
}
bool Space::shape_shapeAndMoveTypeCanEnterEverFrom(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from) const
{
	assert(shape_anythingCanEnterEver(from));
	if(!shape_moveTypeCanEnterFrom(point, moveType, from))
		return false;
	const Facing4 facing = from.getFacingTwords(point);
	return shape_shapeAndMoveTypeCanEnterEverWithFacing(point, shape, moveType, facing);
}
bool Space::shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from, const OccupiedSpaceForHasShape& occupied) const
{
	assert(shape_anythingCanEnterEver(from));
	if(!shape_shapeAndMoveTypeCanEnterEverFrom(point, shape, moveType, from))
		return false;
	return shape_canEnterCurrentlyFrom(point, shape, from, occupied);
}
bool Space::shape_shapeAndMoveTypeCanEnterEverWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing) const
{
	const Cuboid spaceBoundry = boundry();
	if(!shape_anythingCanEnterEver(point) || !shape_moveTypeCanEnter(point, moveType))
		return false;
	const Offset3D pointOffset = point.toOffset();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		if(!shape_anythingCanEnterEver(otherPoint))
			return false;
	}
	return true;
}
bool Space::shape_canEnterCurrentlyWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const
{
	assert(shape_canFitEver(point, shape, facing));
	const Cuboid spaceBoundry = boundry();
	const Offset3D pointOffset = point.toOffset();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		assert(shape_anythingCanEnterEver(otherPoint));
		if(occupied.contains(otherPoint))
			continue;
		if(!shape_anythingCanEnterCurrently(otherPoint) || m_dynamicVolume.queryGetOne(otherPoint) + pair.volume > Config::maxPointVolume)
			return false;
	}
	return true;
}
bool Space::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const
{
	assert(shape_anythingCanEnterEver(point));
	Cuboid spaceBoundry = boundry();
	const Offset3D pointOffset = point.toOffset();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		if(occupied.contains(otherPoint))
			continue;
		if(
			!shape_anythingCanEnterEver(otherPoint) ||
			!shape_anythingCanEnterCurrently(otherPoint) ||
			m_dynamicVolume.queryGetOne(otherPoint) + pair.volume > Config::maxPointVolume ||
			!shape_moveTypeCanEnter(otherPoint, moveType) ||
			(pair.offset.z() != 0 && !pointFeature_multiTileCanEnterAtNonZeroZOffset(otherPoint))
		)
			return false;
	}
	return true;
}
bool Space::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedSpaceForHasShape& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(point, shape, moveType, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(point, shape, moveType, facing, occupied))
			return true;
	return false;
}
bool Space::shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_shapeAndMoveTypeCanEnterEverWithFacing(point, shape, moveType, Facing4::North);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_shapeAndMoveTypeCanEnterEverWithFacing(point, shape, moveType, facing))
			return true;
	return false;
}
Facing4 Space::shape_canEnterEverWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType) const
{
	if(shape_moveTypeCanEnter(point, moveType))
		for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
			if(shape_canFitEver(point, shape, facing))
				return facing;
	return Facing4::Null;
}
Facing4 Space::shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedSpaceForHasShape& occupied) const
{
	if(shape_moveTypeCanEnter(point, moveType))
		for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
			if(shape_canFitEverOrCurrentlyDynamic(point, shape, facing, occupied))
				return facing;
	return Facing4::Null;
}
Facing4 Space::shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedSpaceForHasShape& occupied) const
{
	if(shape_moveTypeCanEnter(point, moveType))
		for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
			if(shape_canFitEverOrCurrentlyStatic(point, shape, facing, occupied))
				return facing;
	return Facing4::Null;
}
bool Space::shape_moveTypeCanEnterFrom(const Point3D& point, const MoveTypeId& moveType, const Point3D& from) const
{
	assert(shape_anythingCanEnterEver(point));
	assert(shape_moveTypeCanEnter(point, moveType));
	for(auto& [fluidType, volume] : MoveType::getSwim(moveType))
	{
		// Can travel within and enter liquid from any angle.
		if(fluid_volumeOfTypeContains(point, fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(fluid_volumeOfTypeContains(from, fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(point.z() == from.z())
		return true;
	// Cannot go up if:
	if(point.z() > from.z() && !pointFeature_canEnterFromBelow(point))
		return false;
	// Cannot go down if:
	if(point.z() < from.z() && !pointFeature_canEnterFromAbove(point, from))
		return false;
	// Can enter from any angle if flying or climbing.
	if(MoveType::getFly(moveType) || MoveType::getClimb(moveType) > 0)
		return true;
	// Can go up if from contains a ramp or stairs.
	if(point.z() > from.z() && (pointFeature_contains(from, PointFeatureTypeId::Ramp) || pointFeature_contains(from, PointFeatureTypeId::Stairs)))
		return true;
	// Can go down if this contains a ramp or stairs.
	if(point.z() < from.z() && (pointFeature_contains(point, PointFeatureTypeId::Ramp) ||
					pointFeature_contains(point, PointFeatureTypeId::Stairs)
	))
		return true;
	return false;
}
bool Space::shape_moveTypeCanEnter(const Point3D& point, const MoveTypeId& moveType) const
{
	assert(shape_anythingCanEnterEver(point));
	const auto& fluidDataSet = m_fluid.queryGetOne(point);
	// Floating.
	if(MoveType::getFloating(moveType) && !fluidDataSet.empty())
		// Any floating move type can potentailly float in any amount of any type of fluid.
		return true;
	// Swiming.
	for(const FluidData& fluidData : fluidDataSet)
	{
		auto found = MoveType::getSwim(moveType).find(fluidData.type);
		if(found != MoveType::getSwim(moveType).end() && found.second() <= fluidData.volume)
		{
			// Can swim in fluid, check breathing requirements
			if(MoveType::getBreathless(moveType))
				return true;
			if(shape_moveTypeCanBreath(point, moveType))
				return true;
			if(point.z() != m_sizeZ - 1)
			{
				const Point3D above = point.above();
				if(shape_anythingCanEnterEver(above) && shape_moveTypeCanBreath(above, moveType))
					return true;
			}
		}
	}
	// Not swimming or floating and fluid level is too high.
	CollisionVolume total = m_totalFluidVolume.queryGetOne(point);
	if(!total.exists())
		total = CollisionVolume::create(0);
	if(total > Config::maxPointVolume / 2)
		return false;
	// Fly can always enter if fluid level isn't preventing it.
	if(MoveType::getFly(moveType))
		return true;
	// Walk can enter only if can stand in or if also has climb2 and there is a isSupport() point adjacent.
	if(MoveType::getWalk(moveType))
	{
		if(shape_canStandIn(point))
			return true;
		else

			if(MoveType::getClimb(moveType) < 2)
				return false;
			else
			{
				// Only climb2 moveTypes can enter.
				for(Point3D otherPoint : getAdjacentOnSameZLevelOnly(point))
					//TODO: check for climable features?
					if(isSupport(otherPoint))
						return true;
				return false;
			}
	}
	return false;
}
bool Space::shape_moveTypeCanBreath(const Point3D& point, const MoveTypeId& moveType) const
{
	if(m_totalFluidVolume.queryGetOneOr(point, {0}) < Config::maxPointVolume && !MoveType::getOnlyBreathsFluids(moveType))
		return true;
	for(const FluidData& fluidData : m_fluid.queryGetOne(point))
		//TODO: Minimum volume should probably be scaled by body size somehow.
		if(MoveType::getBreathableFluids(moveType).contains(fluidData.type) && fluidData.volume >= Config::minimumVolumeOfFluidToBreath)
			return true;
	return false;
}
MoveCost Space::shape_moveCostFrom(const Point3D& point, const MoveTypeId& moveType, const Point3D& from) const
{
	assert(shape_anythingCanEnterEver(point));
	assert(shape_anythingCanEnterEver(from));
	if(MoveType::getFly(moveType))
		return Config::baseMoveCost;
	for(auto& [fluidType, volume] : MoveType::getSwim(moveType))
		if(fluid_volumeOfTypeContains(point, fluidType) >= volume)
			return Config::baseMoveCost;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(point.z() > from.z() && !pointFeature_contains(from, PointFeatureTypeId::Ramp))
		return Config::goUpMoveCost;
	return Config::baseMoveCost;
}
bool Space::shape_canEnterCurrentlyFrom(const Point3D& point, const ShapeId& shape, const Point3D& other, const OccupiedSpaceForHasShape& occupied) const
{
	const Facing4 facing = other.getFacingTwords(point);
	return shape_canEnterCurrentlyWithFacing(point, shape, facing, occupied);
}
bool Space::shape_canEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_canEnterCurrentlyWithFacing(point, shape, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_canEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return true;
	return false;
}
Facing4 Space::shape_canEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_canEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return facing;
	return Facing4::Null;
}
bool Space::shape_staticCanEnterCurrentlyWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const
{
	return shape_staticShapeCanEnterWithFacing(point, shape, facing, occupied);
}
bool Space::shape_staticCanEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticCanEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return true;
	return false;
}
std::pair<bool, Facing4> Space::shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticCanEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return {true, facing};
	return {false, Facing4::North};
}
bool Space::shape_staticShapeCanEnterWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const
{
	const Offset3D pointOffset = point.toOffset();
	const Cuboid spaceBoundry = boundry();
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		const Offset3D sum = pointOffset + pair.offset;
		if(!spaceBoundry.contains(sum))
			return false;
		const Point3D otherPoint = Point3D::create(sum);
		assert(otherPoint.exists());
		if(occupied.contains(otherPoint))
			continue;
		if((m_staticVolume.queryGetOne(otherPoint) + pair.volume > Config::maxPointVolume))
			return false;
	}
	return true;
}
bool Space::shape_staticShapeCanEnterWithAnyFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_staticShapeCanEnterWithFacing(point, shape, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticShapeCanEnterWithFacing(point, shape, facing, occupied))
			return true;
	return false;
}
CollisionVolume Space::shape_getDynamicVolume(const Point3D& point) const
{
	return m_dynamicVolume.queryGetOneOr(point, CollisionVolume::create(0));
}
CollisionVolume Space::shape_getStaticVolume(const Point3D& point) const
{
	return m_staticVolume.queryGetOneOr(point, CollisionVolume::create(0));
}
Quantity Space::shape_getQuantityOfItemWhichCouldFit(const Point3D& point, const ItemTypeId& itemType) const
{
	CollisionVolume freeVolume = Config::maxPointVolume - m_staticVolume.queryGetOneOr(point, CollisionVolume::create(0));
	return Quantity::create((freeVolume / Shape::getCollisionVolumeAtLocation(ItemType::getShape(itemType))).get());
}
SmallSet<Point3D> Space::shape_getBelowPointsWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing) const
{
	// TODO: SIMD.
	SmallSet<Point3D> output;
	assert(point.z() != 0);
	auto occupiedAt = Shape::getPointsOccupiedAt(shape, *this, point, facing);
	for(const Point3D& point : occupiedAt)
	{
		const Point3D& below = point.below();
		if(!occupiedAt.contains(below))
			output.insert(below);
	}
	return output;
}
std::pair<Point3D, Facing4> Space::shape_getNearestEnterableEverPointWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType)
{
	std::deque<Point3D> openList;
	SmallSet<Point3D> closedList;
	closedList.insert(point);
	if(shape_anythingCanEnterEver(point))
		openList.push_back(point);
	else
	{
		for(const Point3D& adjacent : getAdjacentWithEdgeAndCornerAdjacent(point))
			if(shape_anythingCanEnterEver(adjacent))
			{
				openList.push_back(adjacent);
				closedList.insert(adjacent);
			}
		if(openList.empty())
			return {Point3D::null(), Facing4::Null};
	}
	while(!openList.empty())
	{
		const Point3D candidate = openList.front();
		openList.pop_front();
		const Facing4 facing = shape_canEnterEverWithAnyFacingReturnFacing(candidate, shape, moveType);
		if(facing != Facing4::Null)
			return {candidate, facing};
		for(const Point3D& adjacent : getAdjacentWithEdgeAndCornerAdjacent(candidate))
			if(shape_anythingCanEnterEver(adjacent) && !closedList.contains(adjacent))
			{
				openList.push_back(adjacent);
				closedList.insert(adjacent);
			}
	}
	return {Point3D::null(), Facing4::Null};
}
bool Space::shape_canStandIn(const Point3D& point) const
{
	if(point.z() == 0)
		return false;
	const Point3D otherPoint = point.below();
	return (
		(
			solid_is(otherPoint) ||
			pointFeature_canStandAbove(otherPoint))
		) ||
		pointFeature_canStandIn(point) ||
		m_area.m_decks.pointIsPartOfDeck(point);
}
void Space::shape_addStaticVolume(const Point3D& point, const CollisionVolume& volume)
{
	m_staticVolume.updateAddOne(point, volume);
}
void Space::shape_removeStaticVolume(const Point3D& point, const CollisionVolume& volume)
{
	m_staticVolume.updateSubtractOne(point, volume);
}
void Space::shape_addDynamicVolume(const Point3D& point, const CollisionVolume& volume)
{
	m_dynamicVolume.updateAddOne(point, volume);
}
void Space::shape_removeDynamicVolume(const Point3D& point, const CollisionVolume& volume)
{
	m_dynamicVolume.updateSubtractOne(point, volume);
}