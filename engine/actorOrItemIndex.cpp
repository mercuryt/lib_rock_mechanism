#include "actorOrItemIndex.h"
#include "area.h"
#include "types.h"
#include <compare>
void ActorOrItemIndex::setLocationAndFacing(Area& area, BlockIndex location, Facing facing) const
{
	if(isActor())
		area.getActors().setLocationAndFacing(m_index, location, facing); 
	else
		area.getItems().setLocationAndFacing(m_index, location, facing); 
}
void ActorOrItemIndex::reservable_reserve(Area& area, CanReserve& canReserve, Quantity quantity , std::unique_ptr<DishonorCallback> callback) const
{ 
	if(isActor()) 
		area.getActors().reservable_reserve(m_index, canReserve, quantity, std::move(callback));
	else
		area.getItems().reservable_reserve(m_index, canReserve, quantity, std::move(callback));
}
void ActorOrItemIndex::reservable_unreserve(Area& area, CanReserve& canReserve, Quantity quantity) const
{ 
	if(isActor()) 
		area.getActors().reservable_unreserve(m_index, canReserve, quantity);
	else
		area.getItems().reservable_unreserve(m_index, canReserve, quantity);
}
void ActorOrItemIndex::reservable_maybeUnreserve(Area& area, CanReserve& canReserve, Quantity quantity) const
{ 
	if(isActor()) 
		area.getActors().reservable_maybeUnreserve(m_index, canReserve, quantity);
	else
		area.getItems().reservable_maybeUnreserve(m_index, canReserve, quantity);
}
void ActorOrItemIndex::reservable_unreserveFaction(Area& area, const Faction& faction) const
{ 
	if(isActor()) 
		area.getActors().reservable_unreserveFaction(m_index, faction);
	else
		area.getItems().reservable_unreserveFaction(m_index, faction);
}
void ActorOrItemIndex::unfollow(Area& area) const
{
	if(isActor())
		area.getActors().unfollow(m_index);
	else
		area.getItems().unfollow(m_index);
}
CanLead* ActorOrItemIndex::getCanLead(Area& area) const
{ return isActor() ? &area.getActors().getCanLead(m_index) : &area.getItems().getCanLead(m_index); }
CanFollow* ActorOrItemIndex::getCanFollow(Area& area) const
{ return isActor() ? &area.getActors().getCanFollow(m_index) : &area.getItems().getCanFollow(m_index); }
BlockIndex ActorOrItemIndex::getLocation(const Area& area) const 
{ return isActor() ? area.getActors().getLocation(m_index) : area.getItems().getLocation(m_index); }
std::unordered_set<BlockIndex>& ActorOrItemIndex::getBlocks(Area& area) const
{ return isActor() ? area.getActors().getBlocks(m_index) : area.getItems().getBlocks(m_index); }
bool ActorOrItemIndex::isAdjacent(const Area& area, ActorOrItemIndex other) const
{
	const auto& otherBlocks = other.getBlocks(const_cast<Area&>(area));
	return isActor() ? area.getActors().isAdjacentToAny(m_index, otherBlocks) : area.getItems().isAdjacentToAny(m_index, otherBlocks);
}
bool ActorOrItemIndex::isAdjacentToActor(const Area& area, ActorIndex other) const
{
	return isActor() ? area.getActors().isAdjacentToActor(m_index, other) : area.getItems().isAdjacentToActor(m_index, other);
}
bool ActorOrItemIndex::isAdjacentToItem(const Area& area, ItemIndex item) const
{
	return isActor() ? area.getActors().isAdjacentToItem(m_index, item) : area.getItems().isAdjacentToItem(m_index, item);
}
bool ActorOrItemIndex::isAdjacentToLocation(const Area& area, BlockIndex location) const
{
	return isActor() ? area.getActors().isAdjacentToLocation(m_index, location) : area.getItems().isAdjacentToLocation(m_index, location);
}
const Shape& ActorOrItemIndex::getShape(const Area& area) const { return isActor() ? area.getActors().getShape(m_index) : area.getItems().getShape(m_index); }
const MoveType& ActorOrItemIndex::getMoveType(const Area& area) const{ return isActor() ? area.getActors().getMoveType(m_index) : area.getItems().getMoveType(m_index); }
Facing ActorOrItemIndex::getFacing(const Area& area) const { return isActor() ? area.getActors().getFacing(m_index) : area.getItems().getFacing(m_index); }
Mass ActorOrItemIndex::getMass(const Area& area) const { return isActor() ? area.getActors().getMass(m_index) : area.getItems().getMass(m_index); }
Mass ActorOrItemIndex::getSingleUnitMass(const Area& area) const { return isActor() ? area.getActors().getMass(m_index) : area.getItems().getSingleUnitMass(m_index); }
Volume ActorIndex::getVolume(const Area& area)  const { return isActor() ? area.getActors().getVolume(m_index) : area.getItems().getVolume(m_index); }
bool ActorOrItemIndex::isGeneric(const Area& area) const {return isActor() ? false : area.getItems().isGeneric(m_index); }
Quantity ActorOrItemIndex::reservable_getUnreservedCount(Area& area, const Faction& faction) const
{
	if(isActor()) 
		return area.getActors().reservable_getUnreservedCount(m_index, faction);
	else
		return area.getItems().reservable_getUnreservedCount(m_index, faction);
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
