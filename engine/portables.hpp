#pragma once
#include "portables.h"
#include "actorOrItemIndex.h"
#include "actors/actors.h"
#include "deserializationMemo.h"
#include "index.h"
#include "items/items.h"
#include "area.h"
#include "onDestroy.h"
#include "reservable.h"
#include "simulation.h"
#include "types.h"
#include "moveType.h"
#include "hasShapes.hpp"
#include <cstdint>
#include <memory>

template<class Derived, class Index, class ReferenceIndex>
Portables<Derived, Index, ReferenceIndex>::Portables(Area& area, bool isActors) : HasShapes<Derived, Index>(area), m_isActors(isActors) { }
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::resize(const Index& newSize)
{
	HasShapes<Derived, Index>::resize(newSize);
	m_moveType.resize(newSize);
	m_destroy.resize(newSize);
	m_reservables.resize(newSize);
	m_follower.resize(newSize);
	m_leader.resize(newSize);
	m_carrier.resize(newSize);
	m_referenceData.reserve(newSize);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::moveIndex(const Index& oldIndex, const Index& newIndex)
{
	HasShapes<Derived, Index>::moveIndex(oldIndex, newIndex);
	m_moveType[newIndex] = m_moveType[oldIndex];
	m_destroy[newIndex] = std::move(m_destroy[oldIndex]);
	m_reservables[newIndex] = std::move(m_reservables[oldIndex]);
	m_follower[newIndex] = m_follower[oldIndex];
	m_leader[newIndex] = m_leader[oldIndex];
	m_carrier[newIndex] = m_carrier[oldIndex];
	if(m_carrier[newIndex].exists())
		updateIndexInCarrier(oldIndex, newIndex);
	if(m_follower[newIndex].exists())
	{
		ActorOrItemIndex follower = m_follower[newIndex];
		if(follower.isActor())
		{
			assert(getActors().getLeader(follower.getActor()).get() == oldIndex.get());
			getActors().getLeader(follower.getActor()).updateIndex(newIndex);
		}
		else
		{
			assert(getItems().getLeader(follower.getItem()).get() == oldIndex.get());
			getItems().getLeader(follower.getItem()).updateIndex(newIndex);
		}
	}
	if(m_leader[newIndex].exists())
	{
		ActorOrItemIndex leader = m_leader[newIndex];
		if(leader.isActor())
		{
			assert(getActors().getFollower(leader.getActor()).get() == oldIndex.get());
			getActors().getFollower(leader.getActor()).updateIndex(newIndex);
		}
		else
		{
			assert(getItems().getFollower(leader.getItem()).get() == oldIndex.get());
			getItems().getFollower(leader.getItem()).updateIndex(newIndex);
		}
	}
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::create(const Index& index, const MoveTypeId& moveType, const ShapeId& shape, const FactionId& faction, bool isStatic, const Quantity& quantity)
{
	HasShapes<Derived, Index>::create(index, shape, faction, isStatic);
	m_moveType[index] = moveType;
	//TODO: leave as nullptr to start, create as needed.
	m_reservables[index] = std::make_unique<Reservable>(quantity);
	assert(m_destroy[index] == nullptr);
	assert(m_follower[index].empty());
	assert(m_leader[index].empty());
	assert(m_carrier[index].empty());
	// Corresponding remove in Actors::destroy and Items::destroy.
	m_referenceData.add(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::log(const Index& index) const
{
	std::cout << ", moveType: " << MoveType::getName(m_moveType[index]);
	if(m_follower[index].exists())
		std::cout << ", leading: " << m_follower[index].toString();
	if(m_leader[index].exists())
		std::cout << ", following: " << m_follower[index].toString();
	if(m_carrier[index].exists())
	       	std::cout << ", carrier: " << m_carrier[index].toString();
	if(getLocation(index).exists())
		std::cout << ", location: " << getArea().getBlocks().getCoordinates(getLocation(index)).toString();
}
template<class Derived, class Index, class ReferenceIndex>
ActorOrItemIndex Portables<Derived, Index, ReferenceIndex>::getActorOrItemIndex(const Index& index)
{
	if(m_isActors)
		return ActorOrItemIndex::createForActor(ActorIndex::cast(index));
	else
		return ActorOrItemIndex::createForItem(ItemIndex::cast(index));
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::followActor(const Index& index, const ActorIndex& actor)
{
	Actors& actors = getActors();
	assert(!m_leader[index].exists());
	m_leader[index] = ActorOrItemIndex::createForActor(actor);
	this->maybeUnsetStatic(index);
	assert(!actors.getFollower(actor).exists());
	actors.setFollower(actor, getActorOrItemIndex(index));
	ActorIndex lineLeader = getLineLeader(index);
	actors.lineLead_appendToPath(lineLeader, this->m_location[index]);
	actors.move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::followItem(const Index& index, const ItemIndex& item)
{
	Actors& actors = getActors();
	assert(!m_leader[index].exists());
	m_leader[index] = ActorOrItemIndex::createForItem(item);
	this->maybeUnsetStatic(index);
	assert(!getItems().getFollower(item).exists());
	getItems().getFollower(item) = getActorOrItemIndex(index);
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
	actors.lineLead_appendToPath(lineLeader, this->m_location[index]);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::followPolymorphic(const Index& index, const ActorOrItemIndex& actorOrItem)
{
	if(actorOrItem.isActor())
		followActor(index, ActorIndex::cast(actorOrItem.get()));
	else
		followItem(index, ItemIndex::cast(actorOrItem.get()));
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unfollowActor(const Index& index, const ActorIndex& actor)
{
	assert(!isLeading(index));
	assert(isFollowing(index));
	assert(m_leader[index] == actor.toActorOrItemIndex());
	static const MoveTypeId& moveTypeNone = MoveType::byName("none");
	if(!m_isActors || getMoveType(index) == moveTypeNone)
		this->setStatic(index);
	Actors& actors = getActors();
	if(!actors.isFollowing(actor))
	{
		// Actor is line leader.
		assert(!actors.lineLead_getPath(actor).empty());
		actors.lineLead_clearPath(actor);
	}
	ActorIndex lineLeader = getLineLeader(index);
	actors.unsetFollower(actor, getActorOrItemIndex(index));
	m_leader[index].clear();
	getActors().move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unfollowItem(const Index& index, const ItemIndex& item)
{
	assert(!isLeading(index));
	ActorIndex lineLeader = getLineLeader(index);
	m_leader[index].clear();
	static const MoveTypeId& moveTypeNone = MoveType::byName("none");
	if(!m_isActors || getMoveType(index) == moveTypeNone)
		this->setStatic(index);
	Items& items = getItems();
	items.unsetFollower(item, getActorOrItemIndex(index));
	getActors().move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unfollow(const Index& index)
{
	ActorOrItemIndex leader = m_leader[index];
	assert(leader.isLeading(getArea()));
	if(leader.isActor())
		unfollowActor(index, leader.getActor());
	else
		unfollowItem(index, leader.getItem());
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unfollowIfAny(const Index& index)
{
	if(m_leader[index].exists())
		unfollow(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::leadAndFollowDisband(const Index& index)
{
	assert(isFollowing(index) || isLeading(index));
	ActorOrItemIndex follower = getActorOrItemIndex(index);
	// Go to the back of the line.
	Area& area = getArea();
	while(follower.isLeading(area))
		follower = follower.getFollower(area);
	// Iterate to the second from front unfollowing all followers.
	// TODO: This will cause a redundant updateActualSpeed for each follower.
	ActorOrItemIndex leader;
	while(follower.isFollowing(area))
	{
		leader = follower.getLeader(area);
		follower.unfollow(area);
		follower = leader;
	}
	getActors().lineLead_clearPath(leader.getActor());
		
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::maybeLeadAndFollowDisband(const Index& index)
{
	if(isFollowing(index) || isLeading(index))
		leadAndFollowDisband(index);
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::isFollowing(const Index& index) const
{
	return m_leader[index].exists();
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::isLeading(const Index& index) const
{
	return m_follower[index].exists();
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::isLeadingActor(const Index& index, const ActorIndex& actor) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isActor() && follower.getActor() == actor;
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::isLeadingItem(const Index& index, const ItemIndex& item) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isItem() && follower.getItem() == item;
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::isLeadingPolymorphic(const Index& index, const ActorOrItemIndex& actorOrItem) const
{
	return m_follower[index] == actorOrItem;
}
template<class Derived, class Index, class ReferenceIndex>
Speed Portables<Derived, Index, ReferenceIndex>::lead_getSpeed(const Index& index)
{
	assert(m_isActors);
	const ActorIndex& actorIndex = ActorIndex::cast(index);
	assert(!getActors().isFollowing(actorIndex));
	assert(getActors().isLeading(actorIndex));
	ActorOrItemIndex wrapped = getActorOrItemIndex(index);
	std::vector<ActorOrItemIndex> actorsAndItems;
	Area& area = getArea();
	while(wrapped.exists())
	{
		actorsAndItems.push_back(wrapped);
		if(!wrapped.isLeading(area))
			break;
		wrapped = wrapped.getFollower(area);
	}
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), Mass::create(0));
}
template<class Derived, class Index, class ReferenceIndex>
ActorIndex Portables<Derived, Index, ReferenceIndex>::getLineLeader(const Index& index)
{
	// Recursively traverse to the front of the line and return the leader.
	ActorOrItemIndex leader = m_leader[index];
	if(!leader.exists())
	{
		assert(m_follower[index].exists());
		assert(m_isActors);
		return ActorIndex::cast(index);
	}
	if(leader.isActor())
		return getActors().getLineLeader(leader.getActor());
	else
		return getItems().getLineLeader(leader.getItem());
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::setCarrier(const Index& index, const ActorOrItemIndex& carrier)
{
	assert(!m_carrier[index].exists());
	m_carrier[index] = carrier;
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::maybeSetCarrier(const Index& index, const ActorOrItemIndex& carrier)
{
	if(m_carrier[index] == carrier)
		return;
	setCarrier(index, carrier);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unsetCarrier(const Index& index, const ActorOrItemIndex& carrier)
{
	assert(m_carrier[index] == carrier);
	m_carrier[index].clear();
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::updateIndexInCarrier(const Index& oldIndex, const Index& newIndex)
{
	if(m_carrier[newIndex].isActor())
	{
		// Carrier is actor, either via canPickUp or equipmentSet.
		const ActorIndex& actor = ActorIndex::cast(m_carrier[newIndex].get());
		Actors& actors = getActors();
		if(m_isActors)
		{
			// actor is carrying actor
			ActorIndex oi = ActorIndex::cast(oldIndex);
			ActorIndex ni = ActorIndex::cast(newIndex);
			assert(actors.canPickUp_isCarryingActor(oi, actor));
			actors.canPickUp_updateActorIndex(actor, oi, ni);
		}
		else
		{
			ItemIndex oi = ItemIndex::cast(oldIndex);
			ItemIndex ni = ItemIndex::cast(newIndex);
			if(actors.canPickUp_isCarryingItem(actor, oi))
				actors.canPickUp_updateItemIndex(actor, oi, ni);
		}
	}
	else
	{
		// Carrier is item.
		const ItemIndex& item = ItemIndex::cast(m_carrier[newIndex].get());
		Items& items = getItems();
		assert(ItemType::getInternalVolume(items.getItemType(item)) != 0);
		if(m_isActors)
		{
			ActorIndex oi = ActorIndex::cast(oldIndex);
			ActorIndex ni = ActorIndex::cast(newIndex);
			items.cargo_updateActorIndex(item, oi, ni);
		}
		else
		{
			ItemIndex oi = ItemIndex::cast(oldIndex);
			ItemIndex ni = ItemIndex::cast(newIndex);
			items.cargo_updateItemIndex(item, oi, ni);
		}
	}
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_reserve(const Index& index, CanReserve& canReserve, const Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->reserveFor(canReserve, quantity, std::move(callback));
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_unreserve(const Index& index, CanReserve& canReserve, const Quantity quantity)
{
	m_reservables[index]->clearReservationFor(canReserve, quantity);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_unreserveFaction(const Index& index, const FactionId& faction)
{
	m_reservables[index]->clearReservationsFor(faction);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_maybeUnreserve(const Index& index, CanReserve& canReserve, const Quantity quantity)
{
	m_reservables[index]->maybeClearReservationFor(canReserve, quantity);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_unreserveAll(const Index& index)
{
	m_reservables[index]->clearAll();
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_setDishonorCallback(const Index& index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->setDishonorCallbackFor(canReserve, std::move(callback));
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::reservable_merge(const Index& index, Reservable& other)
{
	m_reservables[index]->merge(other);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::load(const Json& data)
{
	nlohmann::from_json(data, static_cast<HasShapes<Derived, Index>&>(*this));
	data["follower"].get_to(m_follower);
	data["leader"].get_to(m_leader);
	data["moveType"].get_to(m_moveType);
	data["referenceData"].get_to(m_referenceData);
	m_reservables.resize(m_moveType.size());
	Area& area = getArea();
	DeserializationMemo& deserializationMemo = area.m_simulation.getDeserializationMemo();
	assert(data["reservable"].type() == Json::value_t::object);
	for(auto iter = data["reservable"].begin(); iter != data["reservable"].end(); ++iter)
	{
		const Index& index = Index::create(std::stoi(iter.key()));
		const Quantity quantity = iter.value()["maxReservations"].get<Quantity>();
		m_reservables[index] = std::make_unique<Reservable>(quantity);
		uintptr_t address;
		iter.value()["address"].get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables[index].get();
	}
	m_destroy.resize(m_moveType.size());
	assert(data["onDestroy"].type() == Json::value_t::object);
	for(auto iter = data["onDestroy"].begin(); iter != data["onDestroy"].end(); ++iter)
	{
		const Index& index = Index::create(std::stoi(iter.key()));
		m_destroy[index] = std::make_unique<OnDestroy>(iter.value(), deserializationMemo);
		uintptr_t address;
		iter.value().get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables[index].get();
	}
}
template<class Derived, class Index, class ReferenceIndex>
Json Portables<Derived, Index, ReferenceIndex>::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output.update({
		{"follower", m_follower},
		{"leader", m_leader},
		{"onDestroy", Json::object()},
		{"reservable", Json::object()},
		{"moveType", m_moveType},
		{"referenceData", m_referenceData}
	});
	Index i = Index::create(0);
	for(; i < m_moveType.size(); ++i)
	{
		// OnDestroy and Reservable don't serialize any data beyone their old address.
		// To deserialize them we just create empties and store pointers to them in deserializationMemo.
		if(m_destroy[i] != nullptr)
			output["onDestroy"][std::to_string(i.get())] = *m_destroy[i].get();
		if(m_reservables[i] != nullptr)
		{
			//TODO: Reservable::toJson
			Json data;
			data["address"] = reinterpret_cast<uintptr_t>(&*m_reservables[i]);
			data["maxReservations"] = m_reservables[i]->getMaxReservations();
			output["reservable"][std::to_string(i.get())] = data;
		}
	}
	return output;
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::reservable_hasAnyReservations(const Index& index) const
{
	return m_reservables[index]->hasAnyReservations();
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::reservable_exists(const Index& index, const FactionId& faction) const
{
	return m_reservables[index]->hasAnyReservationsWith(faction);
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::reservable_existsFor(const Index& index, const CanReserve& canReserve) const
{
	return m_reservables[index]->hasAnyReservationsFor(canReserve);
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::reservable_isFullyReserved(const Index& index, const FactionId& faction) const
{
	return m_reservables[index]->isFullyReserved(faction);
}
template<class Derived, class Index, class ReferenceIndex>
Quantity Portables<Derived, Index, ReferenceIndex>::reservable_getUnreservedCount(const Index& index, const FactionId& faction) const
{
	return m_reservables[index]->getUnreservedCount(faction);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDestroy_subscribe(const Index& index, HasOnDestroySubscriptions& hasSubscriptions)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>();
	hasSubscriptions.subscribe(*m_destroy[index].get());
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDestroy_subscribeThreadSafe(const Index& index, HasOnDestroySubscriptions& hasSubscriptions)
{
	std::lock_guard<std::mutex> lock(HasOnDestroySubscriptions::m_mutex);
	onDestroy_subscribe(index, hasSubscriptions);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDestroy_unsubscribe(const Index& index, HasOnDestroySubscriptions& hasSubscriptions)
{
	m_destroy[index]->unsubscribe(hasSubscriptions);
	if(m_destroy[index]->empty())
		m_destroy[index] = nullptr;
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDestroy_unsubscribeAll(const Index& index)
{
	m_destroy[index]->unsubscribeAll();
	m_destroy[index] = nullptr;
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDestroy_merge(const Index& index, OnDestroy& other)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>();
	m_destroy[index]->merge(other);
}