#include "actorOrItemIndex.h"
#include "area/area.h"
#include "numericTypes/index.h"
#include "numericTypes/types.h"
#include "actors/actors.h"
#include "items/items.h"
#include "portables.hpp"
#include <compare>
void ActorOrItemIndex::followActor(Area& area, const ActorIndex& actor) const
{
	if(isActor())
		area.getActors().followActor(m_index.toActor(), actor);
	else
		area.getItems().followActor(m_index.toItem(), actor);
}
void ActorOrItemIndex::followItem(Area& area, const ItemIndex& item) const
{
	if(isActor())
		area.getActors().followItem(m_index.toActor(), item);
	else
		area.getItems().followItem(m_index.toItem(), item);
}
void ActorOrItemIndex::followPolymorphic(Area& area, const ActorOrItemIndex& actorOrItem) const
{
	if(actorOrItem.isActor())
		followActor(area, actorOrItem.getActor());
	else
		followItem(area, actorOrItem.getItem());
}
ActorOrItemIndex ActorOrItemIndex::location_set(Area& area, const Point3D& location, const Facing4& facing) const
{
	if(isActor())
	{
		area.getActors().location_set(m_index.toActor(), location, facing);
		return *this;
	}
	else
		return area.getItems().location_set(m_index.toItem(), location, facing).toActorOrItemIndex();
}
void ActorOrItemIndex::location_clear(Area& area) const
{
	if(isActor())
		area.getActors().location_clear(m_index.toActor());
	else
		area.getItems().location_clear(m_index.toItem());
}
void ActorOrItemIndex::location_clearStatic(Area& area) const
{
	if(isActor())
		area.getActors().location_clearStatic(m_index.toActor());
	else
		area.getItems().location_clearStatic(m_index.toItem());
}
void ActorOrItemIndex::reservable_reserve(Area& area, CanReserve& canReserve, const Quantity quantity , std::unique_ptr<DishonorCallback> callback) const
{
	if(isActor())
		area.getActors().reservable_reserve(m_index.toActor(), canReserve, quantity, std::move(callback));
	else
		area.getItems().reservable_reserve(m_index.toItem(), canReserve, quantity, std::move(callback));
}
void ActorOrItemIndex::reservable_unreserve(Area& area, CanReserve& canReserve, const Quantity quantity) const
{
	if(isActor())
		area.getActors().reservable_unreserve(m_index.toActor(), canReserve, quantity);
	else
		area.getItems().reservable_unreserve(m_index.toItem(), canReserve, quantity);
}
void ActorOrItemIndex::reservable_maybeUnreserve(Area& area, CanReserve& canReserve, const Quantity quantity) const
{
	if(isActor())
		area.getActors().reservable_maybeUnreserve(m_index.toActor(), canReserve, quantity);
	else
		area.getItems().reservable_maybeUnreserve(m_index.toItem(), canReserve, quantity);
}
void ActorOrItemIndex::reservable_unreserveFaction(Area& area, const FactionId& faction) const
{
	if(isActor())
		area.getActors().reservable_unreserveFaction(m_index.toActor(), faction);
	else
		area.getItems().reservable_unreserveFaction(m_index.toItem(), faction);
}
void ActorOrItemIndex::unfollow(Area& area) const
{
	if(isActor())
		area.getActors().unfollow(m_index.toActor());
	else
		area.getItems().unfollow(m_index.toItem());
}
void ActorOrItemIndex::move_updateIndividualSpeed(Area& area) const
{
	assert(isActor());
	area.getActors().move_updateIndividualSpeed(m_index.toActor());
}
void ActorOrItemIndex::maybeSetStatic(Area& area) const
{
	if(isActor())
	{
		const ActorIndex& index = m_index.toActor();
		Actors& actors = area.getActors();
		if(!actors.isStatic(index))
			actors.setStatic(index);
	}
	else
	{
		const ItemIndex& index = m_index.toItem();
		Items& items = area.getItems();
		if(!items.isStatic(index))
			items.setStatic(index);
	}
}
void ActorOrItemIndex::setStatic(Area& area) const
{
	if(isActor())
		area.getActors().setStatic(m_index.toActor());
	else
		area.getItems().setStatic(m_index.toItem());
}
std::string ActorOrItemIndex::toString() const
{
	if(empty())
		return "null";
	return (isActor() ? "actor#" : "item#") + std::to_string(m_index.get());
}
ActorOrItemReference ActorOrItemIndex::toReference(Area& area) const
{
	ActorOrItemReference output;
	if(isActor())
		output.setActor(area.getActors().m_referenceData.getReference(m_index.toActor()));
	else
		output.setItem(area.getItems().m_referenceData.getReference(m_index.toItem()));
	return output;
}
bool ActorOrItemIndex::isStatic(Area& area) const
{
	if(isActor())
		return area.getActors().isStatic(m_index.toActor());
	else
		return area.getItems().isStatic(m_index.toItem());
}
bool ActorOrItemIndex::isFollowing(Area& area) const
{
	if(isActor())
		return area.getActors().isFollowing(m_index.toActor());
	else
		return area.getItems().isFollowing(m_index.toItem());
}
bool ActorOrItemIndex::isLeading(Area& area) const
{
	if(isActor())
		return area.getActors().isLeading(m_index.toActor());
	else
		return area.getItems().isLeading(m_index.toItem());
}
ActorOrItemIndex ActorOrItemIndex::getFollower(Area& area) const
{
	if(isActor())
		return area.getActors().getFollower(m_index.toActor());
	else
		return area.getItems().getFollower(m_index.toItem());
}
ActorOrItemIndex ActorOrItemIndex::getLeader(Area& area) const
{
	if(isActor())
		return area.getActors().getLeader(m_index.toActor());
	else
		return area.getItems().getLeader(m_index.toItem());
}
bool ActorOrItemIndex::canEnterCurrentlyFrom(Area& area, const Point3D& destination, const Point3D& origin) const
{
	if(isActor())
	{
		Actors& actors = area.getActors();
		ShapeId shape = actors.getShape(m_index.toActor());
		auto& occupied = actors.getOccupied(m_index.toActor());
		return area.getSpace().shape_canEnterCurrentlyFrom(destination, shape, origin, occupied);
	}
	else
	{
		Items& items = area.getItems();
		ShapeId shape = items.getShape(m_index.toItem());
		auto& occupied = items.getOccupied(m_index.toItem());
		return area.getSpace().shape_canEnterCurrentlyFrom(destination, shape, origin, occupied);
	}
}
bool ActorOrItemIndex::canEnterCurrentlyFromWithOccupied(Area& area, const Point3D& destination, const Point3D& origin, const OccupiedSpaceForHasShape& occupied) const
{
	if(isActor())
	{
		Actors& actors = area.getActors();
		ShapeId shape = actors.getShape(m_index.toActor());
		return area.getSpace().shape_canEnterCurrentlyFrom(destination, shape, origin, occupied);
	}
	else
	{
		Items& items = area.getItems();
		ShapeId shape = items.getShape(m_index.toItem());
		return area.getSpace().shape_canEnterCurrentlyFrom(destination, shape, origin, occupied);
	}
}
Point3D ActorOrItemIndex::getLocation(const Area& area) const
{ return isActor() ? area.getActors().getLocation(m_index.toActor()) : area.getItems().getLocation(m_index.toItem()); }
const OccupiedSpaceForHasShape& ActorOrItemIndex::getOccupied(const Area& area) const
{ return isActor() ? area.getActors().getOccupied(m_index.toActor()) : area.getItems().getOccupied(m_index.toItem()); }
SmallSet<Point3D> ActorOrItemIndex::getAdjacentPoints(Area& area) const
{ return isActor() ? area.getActors().getAdjacentPoints(m_index.toActor()) : area.getItems().getAdjacentPoints(m_index.toItem()); }
bool ActorOrItemIndex::isAdjacent(const Area& area, const ActorOrItemIndex& other) const
{
	const auto& otherPoints = other.getOccupied(const_cast<Area&>(area));
	return isActor() ? area.getActors().isAdjacentToAny(m_index.toActor(), otherPoints) : area.getItems().isAdjacentToAny(m_index.toItem(), otherPoints);
}
bool ActorOrItemIndex::isAdjacentToActor(const Area& area, const ActorIndex& other) const
{
	return isActor() ? area.getActors().isAdjacentToActor(m_index.toActor(), other) : area.getItems().isAdjacentToActor(m_index.toItem(), other);
}
bool ActorOrItemIndex::isAdjacentToItem(const Area& area, const ItemIndex& item) const
{
	return isActor() ? area.getActors().isAdjacentToItem(m_index.toActor(), item) : area.getItems().isAdjacentToItem(m_index.toItem(), item);
}
bool ActorOrItemIndex::isAdjacentToLocation(const Area& area, const Point3D& location) const
{
	return isActor() ? area.getActors().isAdjacentToLocation(m_index.toActor(), location) : area.getItems().isAdjacentToLocation(m_index.toItem(), location);
}
bool ActorOrItemIndex::occupiesPoint(const Area& area, const Point3D& location) const
{
	// TODO: check Space::m_items / Space::m_actors instead?
	return isActor() ? area.getActors().getOccupied(m_index.toActor()).contains(location) : area.getItems().getOccupied(m_index.toItem()).contains(location);
}
ShapeId ActorOrItemIndex::getShape(const Area& area) const { return isActor() ? area.getActors().getShape(m_index.toActor()) : area.getItems().getShape(m_index.toItem()); }
ShapeId ActorOrItemIndex::getCompoundShape(const Area& area) const { return isActor() ? area.getActors().getCompoundShape(m_index.toActor()) : area.getItems().getCompoundShape(m_index.toItem()); }
MoveTypeId ActorOrItemIndex::getMoveType(const Area& area) const{ return isActor() ? area.getActors().getMoveType(m_index.toActor()) : area.getItems().getMoveType(m_index.toItem()); }
Facing4 ActorOrItemIndex::getFacing(const Area& area) const { return isActor() ? area.getActors().getFacing(m_index.toActor()) : area.getItems().getFacing(m_index.toItem()); }
Mass ActorOrItemIndex::getMass(const Area& area) const { return isActor() ? area.getActors().getMass(m_index.toActor()) : area.getItems().getMass(m_index.toItem()); }
Quantity ActorOrItemIndex::getQuantity(const Area& area) const { return isActor() ? Quantity::create(1) : area.getItems().getQuantity(m_index.toItem()); }
Mass ActorOrItemIndex::getSingleUnitMass(const Area& area) const { return isActor() ? area.getActors().getMass(m_index.toActor()) : area.getItems().getSingleUnitMass(m_index.toItem()); }
FullDisplacement ActorOrItemIndex::getVolume(const Area& area) const { return isActor() ? area.getActors().getVolume(m_index.toActor()) : area.getItems().getVolume(m_index.toItem()); }
bool ActorOrItemIndex::isGeneric(const Area& area) const {return isActor() ? false : area.getItems().isGeneric(m_index.toItem()); }
Quantity ActorOrItemIndex::reservable_getUnreservedCount(Area& area, const FactionId& faction) const
{
	if(isActor())
		return area.getActors().reservable_getUnreservedCount(m_index.toActor(), faction);
	else
		return area.getItems().reservable_getUnreservedCount(m_index.toItem(), faction);
}
bool ActorOrItemIndex::reservable_exists(Area& area, const FactionId& faction) const
{
	if(isActor())
		return area.getActors().reservable_exists(m_index.toActor(), faction);
	else
		return area.getItems().reservable_exists(m_index.toItem(), faction);
}
bool ActorOrItemIndex::reservable_hasAny(Area& area) const
{
	if(isActor())
		return area.getActors().reservable_hasAnyReservations(m_index.toActor());
	else
		return area.getItems().reservable_hasAnyReservations(m_index.toItem());
}
//TODO: Try to use bitbashing instead of m_isActor
void ActorOrItemIndex::setActorBit(HasShapeIndex& index) { index.getReference() |= (1u << 31); }
void ActorOrItemIndex::unsetActorBit(HasShapeIndex& index) { index.getReference() &= ~(1u << 31); }
bool ActorOrItemIndex::getActorBit(const HasShapeIndex& index) { return index.get() & (1u<< 31); }
void ActorOrItemIndex::validate([[maybe_unused]] Area& area) const
{
	if(isActor())
		assert(area.getActors().size() > m_index);
	else
		assert(area.getItems().size() > m_index);
}
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
	return output.get();
}
