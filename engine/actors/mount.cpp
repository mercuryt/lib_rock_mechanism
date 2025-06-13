#include "actors.h"
#include "../area/area.h"
#include "../blocks/blocks.h"
#include "../hasShapes.hpp"
BlockIndex Actors::mount_findLocationToMountOn(const ActorIndex& index, const ActorIndex& toMount) const
{
	const Blocks& blocks = m_area.getBlocks();
	const ShapeId& shape = getShape(index);
	const Facing4& facing = getFacing(toMount);
	const auto& occupied = getBlocks(index);
	for(const BlockIndex& block : getBlocksAbove(toMount))
		// Should riders be dynamic or static?
		if(blocks.shape_canFitEverOrCurrentlyDynamic(block, shape, facing, occupied))
			return block;
	return BlockIndex::null();
}
bool Actors::mount_hasPilot(const ActorIndex& actor) const
{
	return mount_getPilot(actor) != ActorIndex::null();
}
ActorIndex Actors::mount_getPilot(const ActorIndex& actor) const
{
	for(const ActorOrItemIndex& onDeck : m_onDeck[actor])
		if(onDeck.isActor())
		{
			const ActorIndex& actor = onDeck.getActor();
			if(m_isPilot[actor])
				return actor;
		}
	return ActorIndex::null();
}
void Actors::mount_do(const ActorIndex& index, const ActorIndex& toMount, const BlockIndex& location, const bool& pilot)
{
	const Facing4& mountFacing = getFacing(toMount);
	location_set(index, location, mountFacing);
	m_isOnDeckOf[index] = ActorOrItemIndex::createForActor(toMount);
	m_onDeck[toMount].insert(ActorOrItemIndex::createForActor(index));
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
void Actors::mount_undo(const ActorIndex& index, const BlockIndex& location, const Facing4& facing)
{
	const ActorIndex& mount = m_isOnDeckOf[index].getActor();
	assert(mount.exists());
	const BlockIndex& previousLocation = m_location[index];
	removeShapeFromCompoundShape(mount, getShape(index), previousLocation, m_facing[index]);
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(location, getShape(index), getMoveType(index), facing));
	location_set(index, location, facing);
	m_isOnDeckOf[index].clear();
	m_onDeck[mount].erase(ActorOrItemIndex::createForActor(index));
	// Update speed will modify mount speed using the mass of onDeck.
	move_updateIndividualSpeed(mount);
	if(m_isPilot[index])
	{
		m_isPilot.maybeUnset(index);
		// Mount checks for next objective now that it is independent.
		objective_maybeDoNext(mount);
	}
}
void Actors::pilotItem_set(const ActorIndex& index, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(m_facing[index] == items.getFacing(item));
	items.pilot_set(item, index);
	m_isPilot.set(index);
	move_setMoveSpeedActual(index, items.vehicle_getSpeed(item));
	m_compoundShape[index] = items.getShape(item);
	m_moveType[index] = items.getMoveType(item);
	m_area.m_hasTerrainFacades.maybeRegisterMoveType(m_moveType[index]);
}
void Actors::pilotItem_unset(const ActorIndex& index)
{
	Items& items = m_area.getItems();
	items.pilot_clear(m_isOnDeckOf[index].getItem());
	m_isPilot.maybeUnset(index);
	move_updateActualSpeed(index);
	m_compoundShape[index] = m_shape[index];
	resetMoveType(index);
}
bool Actors::pilotItem_isPilotingConstructedItem(const ActorIndex& index)
{
	Items& items = m_area.getItems();
	if(!m_isPilot[index])
		return false;
	const ActorOrItemIndex& isPiloting = getIsPiloting(index);
	if(isPiloting.isActor())
		return false;
	const ItemIndex& item = isPiloting.getItem();
	return items.hasConstructedShape(item);
}