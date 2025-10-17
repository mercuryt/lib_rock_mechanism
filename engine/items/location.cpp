#include "items.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../definitions/moveType.h"
#include "../portables.h"

ItemIndex Items::location_set(const ItemIndex& index, const Point3D& location, Facing4 facing)
{
	if(isStatic(index))
		return location_setStatic(index, location, facing);
	else
		return location_setDynamic(index, location, facing);
}
ItemIndex Items::location_setStatic(const ItemIndex& index, const Point3D& location, Facing4 facing)
{
	Space& space = m_area.getSpace();
	#ifndef NDEBUG
		assert(index.exists());
		assert(isStatic(index));
		assert(location.exists());
		assert(m_location[index] != location);
	#endif
	Facing4 previousFacing = m_facing[index];
	if(isGeneric(index))
	{
		// Check for existing generic item to combine with.
		ItemIndex found = space.item_getGeneric(location, getItemType(index), getMaterialType(index));
		if(found.exists() && getLocation(found) == location && isStatic(found) && (!Shape::getIsMultiTile(getShape(index)) || m_facing[found] == m_facing[index]))
			// Return the index of the found item, which may be different then it was before 'index' was destroyed by merge.
			return merge(found, index);
	}
	Point3D previousLocation = m_location[index];
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForItem(index));
		location_clear(index);
	}
	m_location[index] = location;
	m_facing[index] = facing;
	if(m_constructedShape[index] != nullptr)
		m_constructedShape[index]->setLocationAndFacingStatic(m_area, previousFacing, location, facing, m_occupied[index]);
	else
	{
		MapWithCuboidKeys<CollisionVolume> newOccupiedWithVolume = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, facing);
		space.item_recordStatic(newOccupiedWithVolume, index);
		assert(m_occupiedWithVolume[index].empty());
		assert(m_occupied[index].empty());
		// Copy only the cuboids into m_occupied.
		std::ranges::copy(newOccupiedWithVolume.data.m_data | std::views::transform(&std::pair<Cuboid, CollisionVolume>::first), std::back_inserter(m_occupied[index].m_cuboids.m_data));
		m_occupiedWithVolume[index] = std::move(newOccupiedWithVolume);
	}
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
	return index;
}
ItemIndex Items::location_setDynamic(const ItemIndex& index, const Point3D& location, Facing4 facing)
{
	assert(index.exists());
	assert(!isStatic(index));
	Space& space = m_area.getSpace();
	Point3D previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForItem(index));
		location_clear(index);
	}
	m_location[index] = location;
	m_facing[index] = facing;
	auto& occupied = m_occupied[index];
	if(m_constructedShape[index] != nullptr)
		m_constructedShape[index]->setLocationAndFacingDynamic(m_area, previousFacing, location, facing, occupied);
	else
	{
		MapWithCuboidKeys<CollisionVolume> newOccupiedWithVolume = Shape::getCuboidsOccupiedAtWithVolume(m_shape[index], space, location, facing);
		space.item_recordDynamic(newOccupiedWithVolume, index);
		assert(m_occupiedWithVolume[index].empty());
		assert(m_occupied[index].empty());
		// Copy only the cuboids into m_occupied.
		m_occupied[index] = newOccupiedWithVolume.getCuboids();
		m_occupiedWithVolume[index] = std::move(newOccupiedWithVolume);
	}
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
	return index;
}
SetLocationAndFacingResult Items::location_tryToSetNongenericStatic(const ItemIndex& index, const Point3D& location, const Facing4 facing)
{
	assert(!ItemType::getIsGeneric(m_itemType[index]));
	assert(isStatic(index));
	Space& space = m_area.getSpace();
	const Point3D& previousLocation = getLocation(index);
	const Offset3D offset = previousLocation.offsetTo(location);
	const Facing4& previousFacing = getFacing(index);
	// Apply the same shift to the offsets as the offset from the previous location to the new one and subtract the unshifted position.
	MapWithOffsetCuboidKeys<CollisionVolume> offsetCuboidsAndVolumesDelta = Shape::applyOffsetAndRotationAndSubtractOriginal(m_compoundShape[index], offset, previousFacing, facing);
	// Apply the new location to the offsets.
	const MapWithCuboidKeys<CollisionVolume> cuboidsAndVolumesDelta = offsetCuboidsAndVolumesDelta.relativeTo(previousLocation);
	for(const auto& [cuboid, volume] : cuboidsAndVolumesDelta)
		// Don't use shape_anythingCanEnterEverHere because it checks Space::m_dynamic.
		if(space.solid_isAny(cuboid) || space.pointFeature_blocksEntrance(cuboid))
			return SetLocationAndFacingResult::PermanantlyBlocked;
	for(const auto& [cuboid, volume] : cuboidsAndVolumesDelta)
		if(!space.shape_cuboidCanFitCurrentlyStatic(cuboid, volume))
			return SetLocationAndFacingResult::TemporarilyBlocked;
	location_setStatic(index, location, facing);
	return SetLocationAndFacingResult::Success;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSetGenericStatic(const ItemIndex& index, const Point3D& location, const Facing4 facing)
{
	assert(ItemType::getIsGeneric(m_itemType[index]));
	assert(isStatic(index));
	const ItemTypeId& itemType = m_itemType[index];
	Space& space = m_area.getSpace();
	Offset3D rollBackFrom;
	ItemIndex toCombine = space.item_getGeneric(location, itemType, m_solid[index]);
	if(toCombine.exists() && canCombine(toCombine, index))
	{
		toCombine = merge(toCombine, index);
		return {toCombine, SetLocationAndFacingResult::Success};
	}
	const Point3D& previousLocation = getLocation(index);
	const Offset3D offset = previousLocation.offsetTo(location);
	const Facing4& previousFacing = getFacing(index);
	// Apply the same shift to the offsets as the offset from the previous location to the new one and subtract the unshifted position.
	MapWithOffsetCuboidKeys<CollisionVolume> offsetCuboidsAndVolumesDelta = Shape::applyOffsetAndRotationAndSubtractOriginal(m_compoundShape[index], offset, previousFacing, facing);
	// Apply the new location to the offsets.
	const MapWithCuboidKeys<CollisionVolume> cuboidsAndVolumesDelta = offsetCuboidsAndVolumesDelta.relativeTo(previousLocation);
	for(const auto& [cuboid, volume] : cuboidsAndVolumesDelta)
		// Don't use shape_anythingCanEnterEverHere because it checks Space::m_dynamic.
		if(space.solid_isAny(cuboid) || space.pointFeature_blocksEntrance(cuboid))
			return {index, SetLocationAndFacingResult::PermanantlyBlocked};
	for(const auto& [cuboid, volume] : cuboidsAndVolumesDelta)
		if(space.shape_cuboidCanFitCurrentlyStatic(cuboid, volume))
			return {index, SetLocationAndFacingResult::TemporarilyBlocked};
	location_setStatic(index, location, facing);
	return {index, SetLocationAndFacingResult::Success};
}
SetLocationAndFacingResult Items::location_tryToSetDynamic(const ItemIndex& index, const Point3D& location, const Facing4& facing)
{
	assert(!isStatic(index));
	assert(!ItemType::getIsGeneric(m_itemType[index]));
	Space& space = m_area.getSpace();
	// Apply the same shift to the offsets as the offset from the previous location to the new one and subtract the unshifted position.
	const Point3D& previousLocation = getLocation(index);
	const Offset3D offset = previousLocation.offsetTo(location);
	const Facing4& previousFacing = getFacing(index);
	// Apply the same shift to the offsets as the offset from the previous location to the new one and subtract the unshifted position.
	MapWithOffsetCuboidKeys<CollisionVolume> offsetCuboidsAndVolumesDelta = Shape::applyOffsetAndRotationAndSubtractOriginal(m_compoundShape[index], offset, previousFacing, facing);
	// Apply the new location to the offsets.
	const MapWithCuboidKeys<CollisionVolume> cuboidsAndVolumesDelta = offsetCuboidsAndVolumesDelta.relativeTo(previousLocation);
	for(const auto& [cuboid, volume] : cuboidsAndVolumesDelta)
		// Don't use shape_anythingCanEnterEverHere because it checks Space::m_dynamic.
		if(space.solid_isAny(cuboid) || space.pointFeature_blocksEntrance(cuboid))
			return SetLocationAndFacingResult::PermanantlyBlocked;
	for(const auto& [cuboid, volume] : cuboidsAndVolumesDelta)
		if(space.shape_cuboidCanFitCurrentlyDynamic(cuboid, volume))
			return SetLocationAndFacingResult::TemporarilyBlocked;
	location_setDynamic(index, location, facing);
	return SetLocationAndFacingResult::Success;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSetStatic(const ItemIndex& index, const Point3D& location, const Facing4& facing)
{
	std::pair<ItemIndex, SetLocationAndFacingResult> output;
	if(isGeneric(index))
		// Static generic.
		output = location_tryToSetGenericStatic(index, location, facing);
	else
		// Static nongeneric.
		output = {index, location_tryToSetNongenericStatic(index, location, facing)};
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSet(const ItemIndex& index, const Point3D& location, const Facing4& facing)
{
	if(isStatic(index))
		return location_tryToSetStatic(index, location, facing);
	else
		return {index, location_tryToSetDynamic(index, location, facing)};
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToMoveToStatic(const ItemIndex& index, const Point3D& location)
{
	assert(hasLocation(index));
	assert(isStatic(index));
	const Point3D previousLocation = getLocation(index);
	assert(previousLocation.isAdjacentTo(location));
	const Facing4 facing = previousLocation.getFacingTwords(location);
	if(isGeneric(index))
		// Static generic.
		return location_tryToSetGenericStatic(index, location, facing);
	else
		// Static nongeneric.
		return {index, location_tryToSetNongenericStatic(index, location, facing)};
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToMoveToDynamic(const ItemIndex& index, const Point3D& location)
{
	assert(hasLocation(index));
	assert(!isStatic(index));
	const Point3D previousLocation = getLocation(index);
	assert(previousLocation.isAdjacentTo(location));
	const Facing4 facing = previousLocation.getFacingTwords(location);
	return {index, location_tryToSetDynamic(index, location, facing)};
}
void Items::location_clear(const ItemIndex& index)
{
	if(isStatic(index))
		location_clearStatic(index);
	else
		location_clearDynamic(index);
}
void Items::location_clearStatic(const ItemIndex& index)
{
	assert(isStatic(index));
	assert(m_location[index].exists());
	Point3D location = m_location[index];
	Space& space = m_area.getSpace();
	std::unique_ptr<ConstructedShape>& shapePtr = m_constructedShape[index];
	if(shapePtr != nullptr)
		shapePtr->recordAndClearStatic(m_area, m_occupied[index], location);
	else
		space.item_eraseStatic(m_occupiedWithVolume[index], index);
	m_location[index].clear();
	m_occupied[index].clear();
	m_occupiedWithVolume[index].clear();
	if(space.isExposedToSky(location))
		m_onSurface.maybeUnset(index);
}
void Items::location_clearDynamic(const ItemIndex& index)
{
	assert(!isStatic(index));
	assert(m_location[index].exists());
	Point3D location = m_location[index];
	Space& space = m_area.getSpace();
	std::unique_ptr<ConstructedShape>& shapePtr = m_constructedShape[index];
	if(shapePtr != nullptr)
		shapePtr->recordAndClearDynamic(m_area, m_occupied[index], location);
	else
		space.item_eraseDynamic(m_occupiedWithVolume[index], index);
	m_location[index].clear();
	m_occupied[index].clear();
	m_occupiedWithVolume[index].clear();
	if(space.isExposedToSky(location))
		m_onSurface.maybeUnset(index);
}
bool Items::location_canEnterEverWithFacing(const ItemIndex& index, const Point3D& location, const Facing4& facing) const
{
	return m_area.getSpace().shape_shapeAndMoveTypeCanEnterEverWithFacing(location, m_compoundShape[index], m_moveType[index], facing);
}
bool Items::location_canEnterCurrentlyWithFacing(const ItemIndex& index, const Point3D& location, const Facing4& facing) const
{
	return m_area.getSpace().shape_canEnterCurrentlyWithFacing(location, m_compoundShape[index], facing, m_occupied[index]);
}
bool Items::location_canEnterEverFrom(const ItemIndex& index, const Point3D& location, const Point3D& previous) const
{
	return m_area.getSpace().shape_shapeAndMoveTypeCanEnterEverFrom(location, m_compoundShape[index], m_moveType[index], previous);
}
bool Items::location_canEnterCurrentlyFrom(const ItemIndex& index, const Point3D& location, const Point3D& previous) const
{
	return m_area.getSpace().shape_canEnterCurrentlyFrom(location, m_compoundShape[index], previous, m_occupied[index]);
}