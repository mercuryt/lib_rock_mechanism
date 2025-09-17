#include "actors.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../portables.h"

void Actors::location_set(const ActorIndex& index, const Point3D& location, const Facing4 facing)
{
	if(isStatic(index))
		location_setStatic(index, location, facing);
	else
		location_setDynamic(index, location, facing);
}
void Actors::location_setStatic(const ActorIndex& index, const Point3D& location, const Facing4 facing)
{
	assert(location.exists());
	assert(facing != Facing4::Null);
	assert(isStatic(index));
	Space& space = m_area.getSpace();
	Point3D previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	SmallMap<Point3D, std::unique_ptr<DishonorCallback>> reservableData;
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
		location_clearStatic(index);
	}
	m_location[index] = location;
	m_facing[index] = facing;
	assert(m_occupied[index].empty());
	assert(m_occupiedWithVolume[index].empty());
	m_occupiedWithVolume[index] = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, facing);
	space.actor_recordStatic(m_occupiedWithVolume[index], index);
	m_occupied[index] = m_occupiedWithVolume[index].getCuboids();
	// Record in vision facade if has location and can currently see.
	vision_maybeUpdateLocation(index, location);
	// TODO: redundant with location_clear also calling getReference.
	m_area.m_octTree.record(m_area, getReference(index));
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
}
void Actors::location_setDynamic(const ActorIndex& index, const Point3D& location, const Facing4 facing)
{
	assert(location.exists());
	assert(facing != Facing4::Null);
	assert(!isStatic(index));
	Space& space = m_area.getSpace();
	Point3D previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	SmallMap<Point3D, std::unique_ptr<DishonorCallback>> reservableData;
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
		location_clearDynamic(index);
	}
	m_location[index] = location;
	m_facing[index] = facing;
	assert(m_occupied[index].empty());
	assert(m_occupiedWithVolume[index].empty());
	m_occupiedWithVolume[index] = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, facing);
	space.actor_recordDynamic(m_occupiedWithVolume[index], index);
	m_occupied[index] = m_occupiedWithVolume[index].getCuboids();
	// Record in vision facade if has location and can currently see.
	vision_maybeUpdateLocation(index, location);
	// TODO: redundant with location_clear also calling getReference.
	m_area.m_octTree.record(m_area, getReference(index));
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
}
// Used when item already has a location, rolls back position on failure.
SetLocationAndFacingResult Actors::location_tryToMoveToStatic(const ActorIndex& index, const Point3D& location)
{
	const Point3D previousLocation = getLocation(index);
	const Facing4 previousFacing = getFacing(index);
	const Facing4 facing = previousLocation.getFacingTwords(location);
	DeckRotationData deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
	location_clear(index);
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
	{
		SetLocationAndFacingResult deckSetLocationResult = deckRotationData.tryToReinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
		if(deckSetLocationResult == SetLocationAndFacingResult::Success)
			onSetLocation(index, previousLocation, previousFacing);
		else
			output = deckSetLocationResult;
	}
	// Not using else here: output.second may have changed due to deck occupants reinstance failing.
	if(output != SetLocationAndFacingResult::Success)
	{
		// Rollback.
		[[maybe_unused]] const SetLocationAndFacingResult status = location_tryToSetDynamicInternal(index, previousLocation, previousFacing);
		assert(status == SetLocationAndFacingResult::Success);
	}
	return output;
}
SetLocationAndFacingResult Actors::location_tryToMoveToDynamic(const ActorIndex& index, const Point3D& location)
{
	const Point3D previousLocation = getLocation(index);
	const Facing4 previousFacing = getFacing(index);
	const Facing4 facing = previousLocation.getFacingTwords(location);
	DeckRotationData deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
	location_clear(index);
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output != SetLocationAndFacingResult::Success)
		// Rollback.
		location_setDynamic(index, previousLocation, previousFacing);
	else
	{
		deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
		onSetLocation(index, previousLocation, previousFacing);
	}
	return output;
}
// Used when item does not have a location.
SetLocationAndFacingResult Actors::location_tryToSet(const ActorIndex& index, const Point3D& location, const Facing4& facing)
{
	if(isStatic(index))
		return location_tryToSetStatic(index, location, facing);
	else
		return location_tryToSetDynamic(index, location, facing);
}
SetLocationAndFacingResult Actors::location_tryToSetStatic(const ActorIndex& index, const Point3D& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	SetLocationAndFacingResult output = location_tryToSetStaticInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
		onSetLocation(index, Point3D::null(), Facing4::Null);
	return output;
}
SetLocationAndFacingResult Actors::location_tryToSetDynamic(const ActorIndex& index, const Point3D& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
		onSetLocation(index, Point3D::null(), Facing4::Null);
	return output;
}
SetLocationAndFacingResult Actors::location_tryToSetStaticInternal(const ActorIndex& index, const Point3D& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	assert(isStatic(index));
	Space& space = m_area.getSpace();
	MapWithCuboidKeys<CollisionVolume> cuboidsAndVolumes = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, facing);
	const CuboidSet& occupied = m_occupied[index];
	cuboidsAndVolumes.removeContainedAndFragmentInterceptedAll(occupied);
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		// Don't use shape_anythingCanEnterEverHere because it checks Space::m_dynamic.
		if(space.solid_isAny(cuboid) || space.pointFeature_blocksEntrance(cuboid))
			return SetLocationAndFacingResult::PermanantlyBlocked;
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		if(space.shape_cuboidCanFitCurrentlyStatic(cuboid, volume))
			return SetLocationAndFacingResult::TemporarilyBlocked;
	location_setStatic(index, location, facing);
	return SetLocationAndFacingResult::Success;
}
SetLocationAndFacingResult Actors::location_tryToSetDynamicInternal(const ActorIndex& index, const Point3D& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	assert(!isStatic(index));
	Space& space = m_area.getSpace();
	MapWithCuboidKeys<CollisionVolume> cuboidsAndVolumes = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, facing);
	const CuboidSet& occupied = m_occupied[index];
	cuboidsAndVolumes.removeContainedAndFragmentInterceptedAll(occupied);
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		// Don't use shape_anythingCanEnterEverHere because it checks Space::m_dynamic.
		if(space.solid_isAny(cuboid) || space.pointFeature_blocksEntrance(cuboid))
			return SetLocationAndFacingResult::PermanantlyBlocked;
	for(const auto& [cuboid, volume] : cuboidsAndVolumes)
		if(space.shape_cuboidCanFitCurrentlyDynamic(cuboid, volume))
			return SetLocationAndFacingResult::TemporarilyBlocked;
	location_setDynamic(index, location, facing);
	return SetLocationAndFacingResult::Success;
}
void Actors::location_clear(const ActorIndex& index)
{
	if(isStatic(index))
		location_clearStatic(index);
	else
		location_clearDynamic(index);
}
void Actors::location_clearStatic(const ActorIndex& index)
{
	assert(isStatic(index));
	assert(m_location[index].exists());
	Point3D location = m_location[index];
	m_area.m_octTree.erase(m_area, getReference(index));
	auto& space = m_area.getSpace();
	space.actor_eraseStatic(m_occupiedWithVolume[index], index);
	m_location[index].clear();
	m_facing[index] = Facing4::Null;
	m_occupied[index].clear();
	m_occupiedWithVolume[index].clear();
	if(space.isExposedToSky(location))
		m_onSurface.maybeUnset(index);
	move_pathRequestMaybeCancel(index);
}
void Actors::location_clearDynamic(const ActorIndex& index)
{
	assert(!isStatic(index));
	assert(m_location[index].exists());
	Point3D location = m_location[index];
	m_area.m_octTree.erase(m_area, getReference(index));
	auto& space = m_area.getSpace();
	space.actor_eraseDynamic(m_occupiedWithVolume[index], index);
	m_location[index].clear();
	m_facing[index] = Facing4::Null;
	m_occupied[index].clear();
	m_occupiedWithVolume[index].clear();
	if(space.isExposedToSky(location))
		m_onSurface.maybeUnset(index);
	move_pathRequestMaybeCancel(index);
}
bool Actors::location_canEnterEverWithFacing(const ActorIndex& index, const Point3D& point, const Facing4& facing)
{
	return m_area.getSpace().shape_shapeAndMoveTypeCanEnterEverWithFacing(point, m_compoundShape[index], m_moveType[index], facing);
}
bool Actors::location_canEnterEverWithAnyFacing(const ActorIndex& index, const Point3D& point)
{
	return location_canEnterEverWithAnyFacingReturnFacing(index, point) != Facing4::Null;
}
Facing4 Actors::location_canEnterEverWithAnyFacingReturnFacing(const ActorIndex& index, const Point3D& point)
{
	for(auto facing = Facing4::North; facing != Facing4::Null; facing = Facing4((int)facing + 1))
		if(location_canEnterEverWithFacing(index, point, facing))
			return facing;
	return Facing4::Null;
}