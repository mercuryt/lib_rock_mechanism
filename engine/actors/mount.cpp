#include "actors.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../items/items.h"
Point3D Actors::mount_findLocationToMountOn(ActorIndex index, ActorIndex toMount) const
{
	const Space& space = m_area.getSpace();
	const ShapeId shape = getShape(index);
	const Facing4& facing = getFacing(toMount);
	const auto& occupied = getOccupied(index);
	for(const Cuboid cuboid : getCuboidsAbove(toMount))
		for(const Point3D point : cuboid)
			// Should riders be dynamic or static?
			if(space.shape_canFitEverOrCurrentlyDynamic(point, shape, facing, occupied))
				return point;
	return Point3D::null();
}
bool Actors::mount_hasPilot(ActorIndex actor) const
{
	return mount_getPilot(actor) != ActorIndex::null();
}
ActorIndex Actors::mount_getPilot(ActorIndex actor) const
{
	for(const ActorOrItemIndex& onDeck : m_onDeck[actor])
		if(onDeck.isActor())
		{
			ActorIndex actorOnDeck = onDeck.getActor();
			if(m_isPilot[actorOnDeck])
				return actorOnDeck;
		}
	return ActorIndex::null();
}
void Actors::mount_do(ActorIndex index, ActorIndex toMount, Point3D location, bool pilot)
{
	const Facing4& mountFacing = getFacing(toMount);
	if(!location.empty() && location != m_location[index])
		location_set(index, location, mountFacing);
	mount_set(index, toMount);
	// Update speed will modify mount speed using the mass of onDeck.
	move_updateIndividualSpeed(toMount);
	// Mutate the shape of the mount to add the rider.
	// TODO: Shape::mutateAdd multiple?
	addShapeToCompoundShape(toMount, getShape(index), location, mountFacing);
	if(pilot)
	{
		assert(!mount_hasPilot(toMount));
		m_isPilot.set(index);
	}
}
void Actors::mount_undo(ActorIndex index, Point3D location, Facing4 facing)
{
	ActorIndex mount = m_isOnDeckOf[index].getActor();
	assert(mount.exists());
	const Point3D previousLocation = m_location[index];
	removeShapeFromCompoundShape(mount, getShape(index), previousLocation, m_facing[index]);
	Space& space = m_area.getSpace();
	assert(space.shape_shapeAndMoveTypeCanEnterEverWithFacing(location, getShape(index), getMoveType(index), facing));
	location_set(index, location, facing);
	mount_unset(index, mount);
	// Update speed will modify mount speed using the mass of onDeck.
	move_updateIndividualSpeed(mount);
	if(m_isPilot[index])
	{
		m_isPilot.maybeUnset(index);
		// Mount checks for next objective now that it is independent.
		objective_maybeDoNext(mount);
	}
}
void Actors::mount_set(ActorIndex index, ActorIndex toMount)
{
	m_isOnDeckOf[index] = ActorOrItemIndex::createForActor(toMount);
	m_onDeck[toMount].insert(ActorOrItemIndex::createForActor(index));
}
void Actors::mount_unset(ActorIndex index, ActorIndex mount)
{
	m_isOnDeckOf[index].clear();
	m_onDeck[mount].erase(ActorOrItemIndex::createForActor(index));
}
void Actors::pilotItem_set(ActorIndex index, const ItemIndex item)
{
	Items& items = m_area.getItems();
	assert(m_facing[index] == items.getFacing(item));
	items.pilot_set(item, index);
	m_isPilot.set(index);
	move_setMoveSpeedActual(index, items.vehicle_getSpeed(item));
	m_compoundShape[index] = items.getShape(item);
	m_moveType[index] = items.getMoveType(item);
	items.maybeUnsetStatic(item);
}
void Actors::pilotItem_unset(ActorIndex index)
{
	Items& items = m_area.getItems();
	items.pilot_clear(m_isOnDeckOf[index].getItem());
	m_isPilot.maybeUnset(index);
	move_updateActualSpeed(index);
	m_compoundShape[index] = m_shape[index];
	resetMoveType(index);
}
bool Actors::pilotItem_isPilotingConstructedItem(ActorIndex index)
{
	Items& items = m_area.getItems();
	if(!m_isPilot[index])
		return false;
	const ActorOrItemIndex isPiloting = getIsPiloting(index);
	if(isPiloting.isActor())
		return false;
	const ItemIndex item = isPiloting.getItem();
	return items.hasConstructedShape(item);
}