#include "space.h"
#include "../definitions/shape.h"
#include "../definitions/itemType.h"
#include "../definitions/moveType.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
bool Space::shape_canFitEverOrCurrentlyDynamic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	const Cuboid spaceBoundry = boundry();
	// Some piece of the shape is not in bounds.
	MapWithCuboidKeys<CollisionVolume> toOccupyWithVolume = Shape::getCuboidsOccupiedAtWithVolume(shape, *this, location, facing);
	CuboidSet toOccupy = toOccupyWithVolume.makeCuboidSet();
	if(!spaceBoundry.contains(toOccupy.boundry()))
		return false;
	toOccupy.removeContainedAndFragmentInterceptedAll(occupied);
	if(m_solid.batchQueryAny(toOccupy))
		return false;
	const auto featureCondition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).blocksEntrance; };
	if(m_features.batchQueryWithConditionAny(toOccupy, featureCondition))
		return false;
	for(const auto& [cuboid, volume] : toOccupyWithVolume)
	{
		if(!shape_anythingCanEnterCurrently(cuboid))
			return false;
		const auto volumeCondition = [&](const CollisionVolume& v){ return volume + v > Config::maxPointVolume; };
		if(m_dynamicVolume.queryAnyWithCondition(cuboid, volumeCondition))
			return false;
	}
	return true;
}
bool Space::shape_canFitEverOrCurrentlyStatic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	const Cuboid spaceBoundry = boundry();
	MapWithCuboidKeys<CollisionVolume> toOccupyWithVolume = Shape::getCuboidsOccupiedAtWithVolume(shape, *this, location, facing);
	CuboidSet toOccupy = toOccupyWithVolume.makeCuboidSet();
	// Some piece of the shape is not in bounds.
	if(!spaceBoundry.contains(toOccupy.boundry()))
		return false;
	toOccupy.removeContainedAndFragmentInterceptedAll(occupied);
	if(m_solid.batchQueryAny(toOccupy))
		return false;
	const auto featureCondition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).blocksEntrance; };
	if(m_features.batchQueryWithConditionAny(toOccupy, featureCondition))
		return false;
	for(const auto& [cuboid, volume] : toOccupyWithVolume)
	{
		if(!shape_anythingCanEnterCurrently(cuboid))
			return false;
		const auto volumeCondition = [&](const CollisionVolume& v){ return volume + v > Config::maxPointVolume; };
		if(m_staticVolume.queryAnyWithCondition(cuboid, volumeCondition))
			return false;
	}
	return true;
}
bool Space::shape_canFitEver(const Point3D& location, const ShapeId& shape, const Facing4& facing) const
{
	// TODO: optimize for single point shape.
	assert(shape_anythingCanEnterEver(location));
	// Check that occupied space is in bounds.
	OffsetCuboid spaceBoundry = offsetBoundry();
	OffsetCuboid toOccupyBoundry = Shape::getOffsetCuboidBoundryWithFacing(shape, facing).relativeToPoint(location);
	if(!spaceBoundry.contains(toOccupyBoundry))
		return false;
	CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(shape, *this, location, facing);
	// Check solid.
	if(m_solid.queryAny(toOccupy))
		return false;
	// Check feature blocks entrance or locked door.
	const auto featureCondition = [&](const PointFeature& feature){
		return PointFeatureType::byId(feature.pointFeatureType).blocksEntrance || (feature.pointFeatureType == PointFeatureTypeId::Door && feature.isLocked());
	};
	if(m_features.queryAnyWithCondition(toOccupy, featureCondition))
		return false;
	// Check if tall shape intersects floor or hatch above ground level.
	bool isFlat = toOccupyBoundry.m_high.z() == toOccupyBoundry.m_low.z();
	if(isFlat)
		return true;
	Cuboid toOccupyBoundryWithoutLowestLevel = Cuboid{Point3D::create(toOccupyBoundry.m_high), Point3D::create(toOccupyBoundry.m_low.above())};
	CuboidSet toOccupyWithoutLowestLevel = toOccupy.intersection(toOccupyBoundryWithoutLowestLevel);
	const auto tallShapeCondition = [&](const PointFeature& feature){
		return
			feature.pointFeatureType == PointFeatureTypeId::Floor ||
			feature.pointFeatureType == PointFeatureTypeId::Hatch ||
			feature.pointFeatureType == PointFeatureTypeId::FloorGrate;
	};
	if(m_features.queryAnyWithCondition(toOccupyWithoutLowestLevel, tallShapeCondition))
		return false;
	return true;
}
bool Space::shape_canFitCurrentlyStatic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	assert(shape_canFitEver(location, shape, facing));
	MapWithCuboidKeys<CollisionVolume> toOccupy = Shape::getCuboidsOccupiedAtWithVolume(shape, *this, location, facing);
	toOccupy.removeContainedAndFragmentInterceptedAll(occupied);
	for(const auto& [cuboid, volume] : toOccupy)
	{
		const auto condition = [&](const CollisionVolume& v){ return volume + v > Config::maxPointVolume; };
		if(m_staticVolume.queryAnyWithCondition(cuboid, condition))
			return false;
	}
	return true;
}
bool Space::shape_canFitCurrentlyDynamic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	assert(shape_canFitEver(location, shape, facing));
	MapWithCuboidKeys<CollisionVolume> toOccupy = Shape::getCuboidsOccupiedAtWithVolume(shape, *this, location, facing);
	toOccupy.removeContainedAndFragmentInterceptedAll(occupied);
	for(const auto& [cuboid, volume] : toOccupy)
	{
		const auto condition = [&](const CollisionVolume& v){ return volume + v > Config::maxPointVolume; };
		if(m_dynamicVolume.queryAnyWithCondition(cuboid, condition))
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
bool Space::shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from, const CuboidSet& occupied) const
{
	assert(shape_anythingCanEnterEver(from));
	if(!shape_shapeAndMoveTypeCanEnterEverFrom(point, shape, moveType, from))
		return false;
	return shape_canEnterCurrentlyFrom(point, shape, from, occupied);
}
bool Space::shape_shapeAndMoveTypeCanEnterEverWithFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing) const
{
	return (
		shape_anythingCanEnterEver(location) &&
		shape_moveTypeCanEnter(location, moveType) &&
		shape_canFitEver(location, shape, facing)
	);
}
bool Space::shape_canEnterCurrentlyWithFacing(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	assert(shape_canFitEver(location, shape, facing));
	MapWithCuboidKeys<CollisionVolume> toOccupy = Shape::getCuboidsOccupiedAtWithVolume(shape, *this, location, facing);
	toOccupy.removeContainedAndFragmentInterceptedAll(occupied);
	for(const auto& [cuboid, volume] : toOccupy)
	{
		if(!shape_anythingCanEnterCurrently(cuboid))
			return false;
		const auto condition = [&](const CollisionVolume& v){ return volume + v > Config::maxPointVolume; };
		if(m_dynamicVolume.queryAnyWithCondition(cuboid, condition))
			return false;
	}
	return true;
}
bool Space::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing, const CuboidSet& occupied) const
{
	assert(shape_anythingCanEnterEver(location));
	if(!shape_moveTypeCanEnter(location, moveType))
		return false;
	OffsetCuboid spaceBoundry = offsetBoundry();
	OffsetCuboid offsetShapeBoundry = Shape::getOffsetCuboidBoundryWithFacing(shape, facing).relativeToPoint(location);
	if(!spaceBoundry.contains(offsetShapeBoundry))
		return false;
	MapWithCuboidKeys<CollisionVolume> toOccupy = Shape::getCuboidsOccupiedAtWithVolume(shape, *this, location, facing);
	Cuboid shapeBoundry = Cuboid::create(offsetShapeBoundry);
	toOccupy.removeContainedAndFragmentInterceptedAll(occupied);
	bool isFlat = shapeBoundry.m_high.z() == shapeBoundry.m_low.z();
	Cuboid toOccupyBoundryWithoutLowestLevel = isFlat ? shapeBoundry : Cuboid{shapeBoundry.m_high, shapeBoundry.m_low.above()};
	for(const auto& [cuboid, volume] : toOccupy)
	{
		if(!shape_anythingCanEnterEver(cuboid) || !shape_anythingCanEnterCurrently(cuboid))
			return false;
		const auto volumeCondition = [&](const CollisionVolume& v){ return volume + v > Config::maxPointVolume; };
		if(m_dynamicVolume.queryAnyWithCondition(cuboid, volumeCondition))
			return false;
		if(!isFlat && cuboid.intersects(toOccupyBoundryWithoutLowestLevel))
		{
			Cuboid intersection = cuboid.intersection(toOccupyBoundryWithoutLowestLevel);
			const auto multiTileShapesBlockedByFeaturesAboveFloorLevelCondition = [&](const PointFeature& feature){ return PointFeatureType::byId(feature.pointFeatureType).blocksMultiTileShapesIfNotAtZeroZOffset; };
			if(m_features.queryAnyWithCondition(intersection, multiTileShapesBlockedByFeaturesAboveFloorLevelCondition))
				return false;
		}
	}
	return true;
}
bool Space::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const CuboidSet& occupied) const
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
Facing4 Space::shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const CuboidSet& occupied) const
{
	if(shape_moveTypeCanEnter(point, moveType))
		for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
			if(shape_canFitEverOrCurrentlyDynamic(point, shape, facing, occupied))
				return facing;
	return Facing4::Null;
}
Facing4 Space::shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const CuboidSet& occupied) const
{
	if(shape_moveTypeCanEnter(point, moveType))
		for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
			if(shape_canFitEverOrCurrentlyStatic(point, shape, facing, occupied))
				return facing;
	return Facing4::Null;
}
bool Space::shape_moveTypeCanEnterFrom(const Point3D& point, const MoveTypeId& moveType, const Point3D& from) const
{
	assert(point.isAdjacentTo(from));
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
	// Go Up.
	if(point.z() > from.z())
	{
		// Can't pass through floors,locked hatches, etc.
		if(!pointFeature_canEnterFromBelow(point))
			return false;
		if(MoveType::getFly(moveType))
			return true;
		// stairs and ramps
		if(MoveType::getStairs(moveType))
		{
			// Can go up if from contains a ramp or stairs.
			// TODO: this should be a query.
			if(pointFeature_contains(from, PointFeatureTypeId::Ramp) || pointFeature_contains(from, PointFeatureTypeId::Stairs))
				return true;
		}
		else
		{
			// Can go up if from contains a ramp.
			if(pointFeature_contains(from, PointFeatureTypeId::Ramp))
				return true;
		}
		const Cuboid& cuboid = from.getAllAdjacentIncludingOutOfBounds();
		if(m_solid.queryAny(cuboid))
		{
			// Any point which is adjacent to a solid point is climable for a climbLevel 2.
			const auto& climbLevel = MoveType::getClimb(moveType);
			if(climbLevel == 2)
				return true;
			if(climbLevel == 1)
			{
				// ClimbLevel 1 only allows one level of vertical travel.
				if(shape_canStandIn(point))
					return true;
			}
		}
	}
	// Go Down.
	if(point.z() < from.z())
	{
		// Can't pass through floors,locked hatches, etc.
		if(!pointFeature_canEnterFromBelow(from))
			return false;
		if(MoveType::getFly(moveType))
			return true;
		// stairs and ramps
		if(MoveType::getStairs(moveType))
		{
			// Can go up if from contains a ramp or stairs.
			// TODO: this should be a query.
			if(pointFeature_contains(point, PointFeatureTypeId::Ramp) || pointFeature_contains(point, PointFeatureTypeId::Stairs))
				return true;
		}
		else
		{
			// Can go up if from contains a ramp.
			if(pointFeature_contains(point, PointFeatureTypeId::Ramp))
				return true;
		}
		const Cuboid& cuboid = point.getAllAdjacentIncludingOutOfBounds();
		if(m_solid.queryAny(cuboid))
		{
			// Any point which is adjacent to a solid point is climable for a climbLevel 2.
			const auto& climbLevel = MoveType::getClimb(moveType);
			if(climbLevel == 2)
				return true;
			if(climbLevel == 1)
			{
				// ClimbLevel 1 only allows one level of vertical travel.
				if(shape_canStandIn(point))
					return true;
			}
		}
	}
	return false;
}
bool Space::shape_moveTypeCanEnter(const Point3D& point, const MoveTypeId& moveType) const
{
	assert(shape_anythingCanEnterEver(point));
	const auto& fluidDataSet = m_fluid.queryGetAll(point);
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
	CollisionVolume total = m_totalFluidVolume.queryGetOneOr(point, {0});
	if(total > Config::maxPointVolume / 2)
		return false;
	// Fly can always enter if fluid level isn't preventing it.
	if(MoveType::getFly(moveType))
		return true;
	// Walk can enter only if can stand in or if also has climb2 and there is a isSupport() point adjacent.
	if(MoveType::getSurface(moveType))
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
	for(const FluidData& fluidData : m_fluid.queryGetAll(point))
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
bool Space::shape_canEnterCurrentlyFrom(const Point3D& point, const ShapeId& shape, const Point3D& other, const CuboidSet& occupied) const
{
	const Facing4 facing = other.getFacingTwords(point);
	return shape_canEnterCurrentlyWithFacing(point, shape, facing, occupied);
}
bool Space::shape_canEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_canEnterCurrentlyWithFacing(point, shape, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_canEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return true;
	return false;
}
Facing4 Space::shape_canEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_canEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return facing;
	return Facing4::Null;
}
bool Space::shape_staticCanEnterCurrentlyWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	return shape_staticShapeCanEnterWithFacing(point, shape, facing, occupied);
}
bool Space::shape_staticCanEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticCanEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return true;
	return false;
}
std::pair<bool, Facing4> Space::shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticCanEnterCurrentlyWithFacing(point, shape, facing, occupied))
			return {true, facing};
	return {false, Facing4::North};
}
bool Space::shape_staticShapeCanEnterWithFacing(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const
{
	return shape_canFitCurrentlyStatic(location, shape, facing, occupied);
}
bool Space::shape_staticShapeCanEnterWithAnyFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const
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
CuboidSet Space::shape_getBelowPointsWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing) const
{
	CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(shape, *this, point, facing);
	CuboidSet output;
	for(const Cuboid& cuboid : toOccupy)
	{
		if(cuboid.m_low.z() == 0)
			continue;
		Cuboid belowFace = cuboid.getFace(Facing6::Below);
		belowFace.shift(Facing6::Below, {1});
		output.add(belowFace);
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
bool Space::shape_cuboidCanFitCurrentlyStatic(const Cuboid& cuboid, const CollisionVolume& volume) const
{
	if(volume == Config::maxPointVolume)
		return m_staticVolume.queryAny(cuboid);
	const auto condition = [&](const CollisionVolume& v) { return v + volume <= Config::maxPointVolume; };
	return m_staticVolume.queryAnyWithCondition(cuboid, condition);
}
bool Space::shape_cuboidCanFitCurrentlyDynamic(const Cuboid& cuboid, const CollisionVolume& volume) const
{
	if(volume == Config::maxPointVolume)
		return m_dynamicVolume.queryAny(cuboid);
	const auto condition = [&](const CollisionVolume& v) { return v + volume <= Config::maxPointVolume; };
	return m_dynamicVolume.queryAnyWithCondition(cuboid, condition);
}
bool Space::shape_canStandIn(const Point3D& point) const
{
	if(point.z() == 0)
		return false;
	const Point3D otherPoint = point.below();
	return (
		(
			solid_isAny(otherPoint) ||
			pointFeature_canStandAbove(otherPoint))
		) ||
		pointFeature_canStandIn(point) ||
		m_area.m_decks.isPartOfDeck(point);
}
void Space::shape_addStaticVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes)
{
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		m_staticVolume.updateAddAll(cuboid, volume);
}
void Space::shape_removeStaticVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes)
{
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		m_staticVolume.updateSubtractAll(cuboid, volume);
}
void Space::shape_addDynamicVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes)
{
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		m_dynamicVolume.updateAddAll(cuboid, volume);
}
void Space::shape_removeDynamicVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes)
{
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		m_dynamicVolume.updateSubtractAll(cuboid, volume);
}