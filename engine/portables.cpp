#include "portables.h"
#include "actors/actors.h"
#include "items/items.h"
#include "area.h"
#include "onDestroy.h"
#include "simulation.h"
#include "types.h"

Portables::Portables(Area& area) : HasShapes(area) { }
void Portables::resize(HasShapeIndex newSize)
{
	m_moveType.resize(newSize);
	m_destroy.resize(newSize);
	m_reservables.resize(newSize);
	m_lead.resize(newSize);
	m_follow.resize(newSize);
}
void Portables::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	assert(indexCanBeMoved(oldIndex));
	std::swap(m_moveType[oldIndex], m_moveType[newIndex]);
}
bool Portables::indexCanBeMoved(HasShapeIndex index) const
{
	return m_destroy.at(index) != nullptr && m_reservables.at(index) != nullptr && m_lead.at(index) != nullptr && m_follow.at(index) != nullptr;
}
void Portables::create(HasShapeIndex index, const MoveType& moveType, const Shape& shape, BlockIndex location, Facing facing, bool isStatic)
{
	HasShapes::create(index, shape, location, facing, isStatic);
	m_moveType.at(index) = &moveType;
}
void Portables::destroy(HasShapeIndex index)
{
	m_destroy.at(index) = nullptr;
	m_reservables.at(index) = nullptr;
	m_lead.at(index) = nullptr;
	m_follow.at(index) = nullptr;
	HasShapes::destroy(index);
}
void Portables::followActor(HasShapeIndex index, ActorIndex actor, bool checkAdjacent)
{
	assert(m_follow.at(index) == nullptr);
	assert(m_area.getActors().m_lead.at(actor) == nullptr);
	CanLead& canLead = *m_area.getActors().m_lead.at(actor).get();
	m_follow.at(index)->follow(m_area, canLead, checkAdjacent);
}
void Portables::followItem(HasShapeIndex index, ItemIndex item, bool checkAdjacent)
{
	assert(m_follow.at(index) == nullptr);
	assert(m_area.getItems().m_lead.at(item) == nullptr);
	CanLead& canLead = *m_area.getItems().m_lead[item].get();
	m_follow.at(index)->follow(m_area, canLead, checkAdjacent);
}
void Portables::followPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem, bool checkAdjacent)
{
	assert(actorOrItem.exists());
	if(actorOrItem.isActor())
		followActor(index, actorOrItem.get(), checkAdjacent);
	else
		followItem(index, actorOrItem.get(), checkAdjacent);
}
void Portables::unfollow(HasShapeIndex index)
{
	ActorOrItemIndex leader = m_follow.at(index)->getLeader();
	if(leader.isActor())
		unfollowActor(index, leader.get());
	else
		unfollowItem(index, leader.get());
}
void Portables::unfollowActor(HasShapeIndex index, ActorIndex actor)
{
	assert(isFollowing(index));
	m_follow.at(index)->unfollow(m_area);
	m_follow.at(index) = nullptr;
	m_area.getActors().m_lead.at(actor) = nullptr;
}
void Portables::unfollowItem(HasShapeIndex index, ItemIndex item)
{
	assert(isFollowing(index));
	m_follow.at(index)->unfollow(m_area);
	m_follow.at(index) = nullptr;
	m_area.getItems().m_lead.at(item) = nullptr;
}
void Portables::leadAndFollowDisband(HasShapeIndex index)
{
	m_follow.at(index)->disband(m_area);
}
bool Portables::isFollowing(HasShapeIndex index) const
{
	if(m_follow.at(index) == nullptr)
		return false;
	assert(m_follow.at(index)->isFollowing());
	return true;
}
bool Portables::isLeading(HasShapeIndex index) const
{
	if(m_lead.at(index) == nullptr)
		return false;
	assert(m_lead.at(index)->isLeading());
	return true;
}
bool Portables::isLeadingActor(HasShapeIndex index, ActorIndex actor) const
{

	if(m_lead.at(index) == nullptr)
		return false;
	return m_lead.at(index)->isLeadingActor(actor);
}
bool Portables::isLeadingItem(HasShapeIndex index, ItemIndex item) const
{

	if(m_lead.at(index) == nullptr)
		return false;
	return m_lead.at(index)->isLeadingItem(item);
}
bool Portables::isLeadingPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem) const
{
	if(m_lead.at(index) == nullptr)
		return false;
	return m_lead.at(index)->isLeadingPolymorphic(actorOrItem);
}
void Portables::reservable_reserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables.at(index)->reserveFor(canReserve, quantity, std::move(callback));
}
void Portables::reservable_unreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity)
{
	m_reservables.at(index)->clearReservationFor(canReserve, quantity);
}
void Portables::reservable_unreserveAll(HasShapeIndex index)
{
	m_reservables.at(index)->clearAll();
	m_reservables.at(index) = nullptr;
}
void Portables::reservable_setDishonorCallback(HasShapeIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables.at(index)->setDishonorCallbackFor(canReserve, std::move(callback));
}
void Portables::reservable_merge(HasShapeIndex index, Reservable& other)
{
	m_reservables[index]->merge(other);
}
bool Portables::reservable_hasAnyReservations(HasShapeIndex index) const
{
	if(m_reservables.at(index) != nullptr)
		assert(m_reservables.at(index)->hasAnyReservations());
	return m_reservables.at(index) != nullptr;
}
bool Portables::reservable_exists(HasShapeIndex index, const Faction& faction) const
{
	if(m_reservables.at(index) == nullptr)
		return false;
	return m_reservables.at(index)->hasAnyReservationsWith(faction);
}
Quantity Portables::reservable_getUnreservedCount(HasShapeIndex index, const Faction& faction) const
{
	return m_reservables.at(index)->getUnreservedCount(faction);
}
void Portables::onDestroy_subscribe(HasShapeIndex index, HasOnDestroySubscriptions& hasSubscriptions)
{
	m_destroy[index]->subscribe(hasSubscriptions);
}
void Portables::onDestroy_unsubscribe(HasShapeIndex index, HasOnDestroySubscriptions& hasSubscriptions)
{
	m_destroy.at(index)->unsubscribe(hasSubscriptions);
	if(m_destroy.at(index)->empty())
		m_destroy.at(index) = nullptr;
}
void Portables::onDestroy_unsubscribeAll(HasShapeIndex index)
{
	m_destroy.at(index)->unsubscribeAll();
	m_destroy.at(index) = nullptr;
}
void Portables::onDestroy_merge(HasShapeIndex index, OnDestroy& other)
{
	m_destroy[index]->merge(other);
}
