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
#include <cstdint>
#include <memory>

Portables::Portables(Area& area, bool isActors) : HasShapes(area), m_isActors(isActors) { }
void Portables::resize(HasShapeIndex newSize)
{
	HasShapes::resize(newSize);
	m_moveType.resize(newSize);
	m_destroy.resize(newSize);
	m_reservables.resize(newSize);
	m_follower.resize(newSize);
	m_leader.resize(newSize);
	m_carrier.resize(newSize);
}
void Portables::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	HasShapes::moveIndex(oldIndex, newIndex);
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
			assert(m_area.getActors().m_leader[follower.get()].get() == oldIndex);
			m_area.getActors().m_leader[follower.get()].updateIndex(newIndex);
		}
		else
		{
			assert(m_area.getItems().m_leader[follower.get()].get() == oldIndex);
			m_area.getItems().m_leader[follower.get()].updateIndex(newIndex);
		}
	}
	if(m_leader[newIndex].exists())
	{
		ActorOrItemIndex leader = m_leader[newIndex];
		if(leader.isActor())
		{
			assert(m_area.getActors().m_follower[leader.get()].get() == oldIndex);
			m_area.getActors().m_follower[leader.get()].updateIndex(newIndex);
		}
		else
		{
			assert(m_area.getItems().m_follower[leader.get()].get() == oldIndex);
			m_area.getItems().m_follower[leader.get()].updateIndex(newIndex);
		}
	}
}
void Portables::create(HasShapeIndex index, MoveTypeId moveType, ShapeId shape, FactionId faction, bool isStatic, Quantity quantity)
{
	HasShapes::create(index, shape, faction, isStatic);
	m_moveType[index] = moveType;
	//TODO: leave as nullptr to start, create as needed.
	m_reservables[index] = std::make_unique<Reservable>(quantity);
	assert(m_destroy[index] == nullptr);
	assert(m_follower[index].empty());
	assert(m_leader[index].empty());
	assert(m_carrier[index].empty());
}
void Portables::log(HasShapeIndex index) const
{
	std::cout 
		<< "moveType: " << MoveType::getName(m_moveType[index])
	       	<< ", leading: " << m_follower[index].toString()
	       	<< ", following: " << m_follower[index].toString()
	       	<< ", carrier: " << m_carrier[index].toString();
}
ActorOrItemIndex Portables::getActorOrItemIndex(HasShapeIndex index)
{
	if(m_isActors)
		return ActorOrItemIndex::createForActor(ActorIndex::cast(index));
	else
		return ActorOrItemIndex::createForItem(ItemIndex::cast(index));
}
void Portables::followActor(HasShapeIndex index, ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	assert(!m_leader[index].exists());
	m_leader[index] = ActorOrItemIndex::createForActor(actor);
	assert(!actors.m_follower[actor].exists());
	actors.m_follower[actor] = getActorOrItemIndex(index);
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
	actors.lineLead_appendToPath(lineLeader, m_location[index]);
}
void Portables::followItem(HasShapeIndex index, ItemIndex item)
{
	Actors& actors = m_area.getActors();
	assert(!m_leader[index].exists());
	m_leader[index] = ActorOrItemIndex::createForItem(item);
	assert(!m_area.getItems().m_follower[item].exists());
	m_area.getItems().m_follower[item] = getActorOrItemIndex(index);
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
	actors.lineLead_appendToPath(lineLeader, m_location[index]);
}
void Portables::followPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem)
{
	if(actorOrItem.isActor())
		followActor(index, ActorIndex::cast(actorOrItem.get()));
	else
		followItem(index, ItemIndex::cast(actorOrItem.get()));
}
void Portables::unfollowActor(HasShapeIndex index, ActorIndex actor)
{
	assert(!isLeading(index));
	assert(isFollowing(index));
	assert(m_leader[index] == actor.toActorOrItemIndex());
	Actors& actors = m_area.getActors();
	if(!isFollowing(actor))
	{
		// Actor is line leader.
		assert(!actors.lineLead_getPath(actor).empty());
		actors.lineLead_clearPath(actor);
	}
	ActorIndex lineLeader = getLineLeader(index);
	actors.m_follower[actor].clear();
	m_leader[index].clear();
	m_area.getActors().move_updateActualSpeed(lineLeader);
}
void Portables::unfollowItem(HasShapeIndex index, ItemIndex item)
{
	assert(!isLeading(index));
	m_area.getItems().m_follower[item].clear();
	ActorIndex lineLeader = getLineLeader(index);
	m_leader[index].clear();
	Items& items = m_area.getItems();
	items.m_follower[item].clear();
	m_area.getActors().move_updateActualSpeed(lineLeader);
}
void Portables::unfollow(HasShapeIndex index)
{
	ActorOrItemIndex leader = m_leader[index];
	assert(leader.isLeading(m_area));
	if(leader.isActor())
		unfollowActor(index, leader.getActor());
	else
		unfollowItem(index, leader.getItem());
}
void Portables::unfollowIfAny(HasShapeIndex index)
{
	if(m_leader[index].exists())
		unfollow(index);
}
void Portables::leadAndFollowDisband(HasShapeIndex index)
{
	assert(isFollowing(index) || isLeading(index));
	ActorOrItemIndex follower = getActorOrItemIndex(index);
	// Go to the back of the line.
	while(follower.isLeading(m_area))
		follower = follower.getFollower(m_area);
	// Iterate to the second from front unfollowing all followers.
	// TODO: This will cause a redundant updateActualSpeed for each follower.
	ActorOrItemIndex leader;
	while(follower.isFollowing(m_area))
	{
		leader = follower.getLeader(m_area);
		follower.unfollow(m_area);
		follower = leader;
	}
	m_area.getActors().lineLead_clearPath(leader.getActor());
		
}
void Portables::maybeLeadAndFollowDisband(HasShapeIndex index)
{
	if(isFollowing(index) || isLeading(index))
		leadAndFollowDisband(index);
}
bool Portables::isFollowing(HasShapeIndex index) const
{
	return m_leader[index].exists();
}
bool Portables::isLeading(HasShapeIndex index) const
{
	return m_follower[index].exists();
}
bool Portables::isLeadingActor(HasShapeIndex index, ActorIndex actor) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isActor() && follower.getActor() == actor;
}
bool Portables::isLeadingItem(HasShapeIndex index, ItemIndex item) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isItem() && follower.getItem() == item;
}
bool Portables::isLeadingPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem) const
{
	return m_follower[index] == actorOrItem;
}
Speed Portables::lead_getSpeed(HasShapeIndex index)
{
	assert(m_isActors);
	ActorIndex actorIndex = ActorIndex::cast(index);
	assert(!m_area.getActors().isFollowing(actorIndex));
	assert(m_area.getActors().isLeading(actorIndex));
	ActorOrItemIndex wrapped = getActorOrItemIndex(index);
	std::vector<ActorOrItemIndex> actorsAndItems;
	while(wrapped.exists())
	{
		actorsAndItems.push_back(wrapped);
		if(!wrapped.isLeading(m_area))
			break;
		wrapped = wrapped.getFollower(m_area);
	}
	return getMoveSpeedForGroupWithAddedMass(m_area, actorsAndItems, Mass::create(0), Mass::create(0));
}
ActorIndex Portables::getLineLeader(HasShapeIndex index)
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
		return m_area.getActors().getLineLeader(leader.get());
	else
		return m_area.getItems().getLineLeader(leader.get());
}
// Class method.
Speed Portables::getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems)
{
	return getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), Mass::create(0));
}
Speed Portables::getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass, Mass addedDeadMass)
{
	Mass rollingMass = addedRollingMass;
	Mass deadMass = addedDeadMass;
	Mass carryMass = Mass::create(0);
	Speed lowestMoveSpeed = Speed::create(0);
	for(ActorOrItemIndex index : actorsAndItems)
	{
		if(index.isItem())
		{
			ItemIndex itemIndex = ItemIndex::cast(index.get());
			Mass mass = area.getItems().getMass(itemIndex);
			static MoveTypeId roll = MoveType::byName("roll");
			if(area.getItems().getMoveType(itemIndex) == roll)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(index.isActor());
			ActorIndex actorIndex = ActorIndex::cast(index.get());
			const Actors& actors = area.getActors();
			if(actors.move_canMove(actorIndex))
			{
				carryMass += actors.getUnencomberedCarryMass(actorIndex);
				Speed moveSpeed = actors.move_getSpeed(actorIndex);
				lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
			}
			else
				deadMass += actors.getMass(actorIndex);
		}
	}
	assert(lowestMoveSpeed != 0);
	Mass totalMass = deadMass + (rollingMass * Config::rollingMassModifier);
	if(totalMass <= carryMass)
		return lowestMoveSpeed;
	float ratio = (float)carryMass.get() / (float)totalMass.get();
	if(ratio < Config::minimumOverloadRatio)
		return Speed::create(0);
	return Speed::create(std::ceil(lowestMoveSpeed.get() * ratio * ratio));
}
void Portables::setCarrier(HasShapeIndex index, ActorOrItemIndex carrier)
{
	assert(!m_carrier[index].exists());
	m_carrier[index] = carrier;
}
void Portables::unsetCarrier(HasShapeIndex index, ActorOrItemIndex carrier)
{
	assert(m_carrier[index] == carrier);
	m_carrier[index].clear();
}
void Portables::updateIndexInCarrier(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	if(m_carrier[newIndex].isActor())
	{
		// Carrier is actor, either via canPickUp or equipmentSet.
		ActorIndex actor = ActorIndex::cast(m_carrier[newIndex].get());
		Actors& actors = m_area.getActors();
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
		ItemIndex item = ItemIndex::cast(m_carrier[newIndex].get());
		Items& items = m_area.getItems();
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
void Portables::reservable_reserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->reserveFor(canReserve, quantity, std::move(callback));
}
void Portables::reservable_unreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity)
{
	m_reservables[index]->clearReservationFor(canReserve, quantity);
}
void Portables::reservable_unreserveFaction(HasShapeIndex index, const FactionId faction)
{
	m_reservables[index]->clearReservationsFor(faction);
}
void Portables::reservable_maybeUnreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity)
{
	m_reservables[index]->maybeClearReservationFor(canReserve, quantity);
}
void Portables::reservable_unreserveAll(HasShapeIndex index)
{
	m_reservables[index]->clearAll();
}
void Portables::reservable_setDishonorCallback(HasShapeIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->setDishonorCallbackFor(canReserve, std::move(callback));
}
void Portables::reservable_merge(HasShapeIndex index, Reservable& other)
{
	m_reservables[index]->merge(other);
}
void Portables::load(const Json& data)
{
	nlohmann::from_json(data, static_cast<Portables&>(*this));
	data["follower"].get_to(m_follower);
	data["leader"].get_to(m_leader);
	data["moveType"].get_to(m_moveType);
	m_reservables.resize(m_moveType.size());
	DeserializationMemo& deserializationMemo = m_area.m_simulation.getDeserializationMemo();
	for(const Json& pair : data["reservable"])
	{
		HasShapeIndex index = pair[0];
		m_reservables[index] = std::make_unique<Reservable>(pair[1]["maxReservations"].get<Quantity>());
		uintptr_t address;
		pair[1]["address"].get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables[index].get();
	}
	m_destroy.resize(m_moveType.size());
	for(const Json& pair : data["onDestroy"])
	{
		HasShapeIndex index = pair[0];
		m_destroy[index] = std::make_unique<OnDestroy>(pair[1], deserializationMemo);
		uintptr_t address;
		pair[1]["address"].get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables[index].get();
	}
}
Json Portables::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output.update({
		{"follower", m_follower},
		{"leader", m_leader},
		{"destroy", Json::object()},
		{"reservable", Json::object()},
		{"moveType", m_moveType},
	});
	HasShapeIndex i = HasShapeIndex::create(0);
	for(; i < m_moveType.size(); ++i)
	{
		// OnDestroy and Reservable don't serialize any data beyone their old address.
		// To deserialize them we just create empties and store pointers to them in deserializationMemo.
		if(m_destroy[i] != nullptr)
			output[i.get()] = *m_destroy[i].get();
		if(m_reservables[i] != nullptr)
			output[i.get()] = reinterpret_cast<uintptr_t>(m_reservables[i].get());
	}
	return output;
}
bool Portables::reservable_hasAnyReservations(HasShapeIndex index) const
{
	return m_reservables[index]->hasAnyReservations();
}
bool Portables::reservable_exists(HasShapeIndex index, const FactionId faction) const
{
	return m_reservables[index]->hasAnyReservationsWith(faction);
}
bool Portables::reservable_isFullyReserved(HasShapeIndex index, const FactionId faction) const
{
	return m_reservables[index]->isFullyReserved(faction);
}
Quantity Portables::reservable_getUnreservedCount(HasShapeIndex index, const FactionId faction) const
{
	return m_reservables[index]->getUnreservedCount(faction);
}
void Portables::onDestroy_subscribe(HasShapeIndex index, HasOnDestroySubscriptions& hasSubscriptions)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>();
	hasSubscriptions.subscribe(*m_destroy[index].get());
}
void Portables::onDestroy_subscribeThreadSafe(HasShapeIndex index, HasOnDestroySubscriptions& hasSubscriptions)
{
	std::lock_guard<std::mutex> lock(HasOnDestroySubscriptions::m_mutex);
	onDestroy_subscribe(index, hasSubscriptions);
}
void Portables::onDestroy_unsubscribe(HasShapeIndex index, HasOnDestroySubscriptions& hasSubscriptions)
{
	m_destroy[index]->unsubscribe(hasSubscriptions);
	if(m_destroy[index]->empty())
		m_destroy[index] = nullptr;
}
void Portables::onDestroy_unsubscribeAll(HasShapeIndex index)
{
	m_destroy[index]->unsubscribeAll();
	m_destroy[index] = nullptr;
}
void Portables::onDestroy_merge(HasShapeIndex index, OnDestroy& other)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>();
	m_destroy[index]->merge(other);
}
