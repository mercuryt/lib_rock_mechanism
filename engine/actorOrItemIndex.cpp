#include "actorOrItemIndex.h"
#include "area.h"
#include "types.h"
#include <compare>
void ActorOrItemIndex::setLocationAndFacing(Area& area, BlockIndex location, Facing facing) const
{
	if(isActor())
		area.m_actors.setLocationAndFacing(m_index, location, facing); 
	else
		area.m_items.setLocationAndFacing(m_index, location, facing); 
}
void ActorOrItemIndex::reservable_reserve(Area& area, CanReserve& canReserve, Quantity quantity , std::unique_ptr<DishonorCallback> callback) const
{ 
	if(isActor()) 
		area.m_actors.reservable_reserve(m_index, canReserve, quantity, std::move(callback));
	else
		area.m_items.reservable_reserve(m_index, canReserve, quantity, std::move(callback));
}
void ActorOrItemIndex::reservable_unreserveFaction(Area& area, const Faction& faction) const
{ 
	if(isActor()) 
		area.m_actors.reservable_unreserveFaction(m_index, faction);
	else
		area.m_items.reservable_unreserveFaction(m_index, faction);
}
void ActorOrItemIndex::unfollow(Area& area) const
{
	if(isActor())
		area.m_actors.unfollow(m_index);
	else
		area.m_items.unfollow(m_index);
}
CanLead* ActorOrItemIndex::getCanLead(Area& area) const
{ return isActor() ? &area.m_actors.getCanLead(m_index) : &area.m_items.getCanLead(m_index); }
CanFollow* ActorOrItemIndex::getCanFollow(Area& area) const
{ return isActor() ? &area.m_actors.getCanFollow(m_index) : &area.m_items.getCanFollow(m_index); }
BlockIndex ActorOrItemIndex::getLocation(Area& area) const 
{ return isActor() ? area.m_actors.getLocation(m_index) : area.m_items.getLocation(m_index); }
std::unordered_set<BlockIndex>& ActorOrItemIndex::getBlocks(Area& area) const
{ return isActor() ? area.m_actors.getBlocks(m_index) : area.m_items.getBlocks(m_index); }
bool ActorOrItemIndex::isAdjacent(Area& area, ActorOrItemIndex other) const
{
	auto otherBlocks = other.getBlocks(area);
	return isActor() ? area.m_actors.isAdjacentToAny(m_index, otherBlocks) : area.m_items.isAdjacentToAny(m_index, otherBlocks);
}
bool ActorOrItemIndex::isAdjacentToActor(Area& area, ActorIndex other) const
{
	return isActor() ? area.m_actors.isAdjacentToActor(m_index, other) : area.m_items.isAdjacentToActor(m_index, other);
}
bool ActorOrItemIndex::isAdjacentToItem(Area& area, ItemIndex item) const
{
	return isActor() ? area.m_actors.isAdjacentToItem(m_index, item) : area.m_items.isAdjacentToItem(m_index, item);
}
bool ActorOrItemIndex::isAdjacentToLocation(Area& area, BlockIndex location) const
{
	return isActor() ? area.m_actors.isAdjacentToLocation(m_index, location) : area.m_items.isAdjacentToLocation(m_index, location);
}
const Shape& ActorOrItemIndex::getShape(Area& area) const { return isActor() ? area.m_actors.getShape(m_index) : area.m_items.getShape(m_index); }
const MoveType& ActorOrItemIndex::getMoveType(Area& area) const{ return isActor() ? area.m_actors.getMoveType(m_index) : area.m_items.getMoveType(m_index); }
Facing ActorOrItemIndex::getFacing(Area& area) const { return isActor() ? area.m_actors.getFacing(m_index) : area.m_items.getFacing(m_index); }
Mass ActorOrItemIndex::getMass(Area& area) const { return isActor() ? area.m_actors.getMass(m_index) : area.m_items.getMass(m_index); }
Mass ActorOrItemIndex::getSingleUnitMass(Area& area) const { return isActor() ? area.m_actors.getMass(m_index) : area.m_items.getSingleUnitMass(m_index); }
bool ActorOrItemIndex::isGeneric(Area& area) const {return isActor() ? false : area.m_items.isGeneric(m_index); }
Quantity ActorOrItemIndex::reservable_getUnreservedCount(Area& area, const Faction& faction) const
{
	if(isActor()) 
		return area.m_actors.reservable_getUnreservedCount(m_index, faction);
	else
		return area.m_items.reservable_getUnreservedCount(m_index, faction);
}
void ActorOrItemIndex::setActorBit(HasShapeIndex& index) { index |= (1u << 31); }
void ActorOrItemIndex::unsetActorBit(HasShapeIndex& index) { index &= ~(1u << 31); }
bool ActorOrItemIndex::getActorBit(HasShapeIndex& index) { index = (index >> 31) & 1u; }
std::strong_ordering ActorOrItemIndex::operator<=>(const ActorOrItemIndex& other) const
{
	assert(isActor() == other.isActor());
	return m_index <=> other.m_index;
}
size_t ActorOrItemIndex::Hash::operator()(const ActorOrItemIndex& actorOrItem) const
{
	HasShapeIndex output = actorOrItem.get();
	if(actorOrItem.isActor())
		ActorOrItemIndex::setActorBit(output);
	return output;
}
