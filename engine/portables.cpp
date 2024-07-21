#include "portables.h"
#include "actorOrItemIndex.h"
#include "actors/actors.h"
#include "items/items.h"
#include "area.h"
#include "onDestroy.h"
#include "reservable.h"
#include "simulation.h"
#include "types.h"
#include "moveType.h"
#include <memory>

Portables::Portables(Area& area) : HasShapes(area) { }
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
	m_moveType.at(newIndex) = m_moveType.at(oldIndex);
	m_destroy.at(newIndex) = std::move(m_destroy.at(oldIndex));
	m_reservables.at(newIndex) = std::move(m_reservables.at(oldIndex));
	m_follower.at(newIndex) = m_follower.at(oldIndex);
	m_leader.at(newIndex) = m_leader.at(oldIndex);
	m_carrier.at(newIndex) = m_carrier.at(oldIndex);
	if(m_carrier.at(newIndex).exists())
		updateIndexInCarrier(oldIndex, newIndex);
	if(m_follower.at(newIndex).exists())
	{
		ActorOrItemIndex follower = m_follower.at(newIndex);
		if(follower.isActor())
		{
			assert(m_area.getActors().m_leader.at(follower.get()).get() == oldIndex);
			m_area.getActors().m_leader.at(follower.get()).updateIndex(newIndex);
		}
		else
		{
			assert(m_area.getItems().m_leader.at(follower.get()).get() == oldIndex);
			m_area.getItems().m_leader.at(follower.get()).updateIndex(newIndex);
		}
	}
	if(m_leader.at(newIndex).exists())
	{
		ActorOrItemIndex leader = m_leader.at(newIndex);
		if(leader.isActor())
		{
			assert(m_area.getActors().m_follower.at(leader.get()).get() == oldIndex);
			m_area.getActors().m_follower.at(leader.get()).updateIndex(newIndex);
		}
		else
		{
			assert(m_area.getItems().m_follower.at(leader.get()).get() == oldIndex);
			m_area.getItems().m_follower.at(leader.get()).updateIndex(newIndex);
		}
	}
}
void Portables::create(HasShapeIndex index, const MoveType& moveType, const Shape& shape, BlockIndex location, Facing facing, bool isStatic)
{
	HasShapes::create(index, shape, location, facing, isStatic);
	m_moveType.at(index) = &moveType;
	assert(m_destroy.at(index) == nullptr);
	assert(m_reservables.at(index) == nullptr);
	assert(!m_follower.at(index).exists());
	assert(!m_leader.at(index).exists());
	assert(!m_carrier.at(index).exists());
}
void Portables::destroy(HasShapeIndex index)
{
	m_destroy.at(index) = nullptr;
	m_reservables.at(index) = nullptr;
	m_follower.at(index).clear();
	HasShapes::destroy(index);
}
ActorOrItemIndex Portables::getActorOrItemIndex(HasShapeIndex index)
{
	if(isActors)
		return ActorOrItemIndex::createForActor(index);
	else
		return ActorOrItemIndex::createForItem(index);
}
void Portables::followActor(HasShapeIndex index, ActorIndex actor)
{
	assert(!m_leader.at(index).exists());
	m_leader.at(index) = ActorOrItemIndex::createForActor(actor);
	assert(!m_area.getActors().m_follower.at(index).exists());
	m_area.getActors().m_follower.at(index) = getActorOrItemIndex(index);
	m_area.getActors().move_updateActualSpeed(getLineLeader(index));
}
void Portables::followItem(HasShapeIndex index, ItemIndex item)
{
	assert(!m_leader.at(index).exists());
	m_leader.at(index) = ActorOrItemIndex::createForItem(item);
	assert(!m_area.getItems().m_follower.at(index).exists());
	m_area.getItems().m_follower.at(index) = getActorOrItemIndex(index);
	m_area.getActors().move_updateActualSpeed(getLineLeader(index));
}
void Portables::followPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem)
{
	if(actorOrItem.isActor())
		followActor(index, actorOrItem.get());
	else
		followItem(index, actorOrItem.get());
}
void Portables::unfollowActor(HasShapeIndex index, ActorIndex actor)
{
	m_area.getActors().m_follower.at(actor).clear();
	m_leader.at(index).clear();
	m_area.getActors().move_updateActualSpeed(getLineLeader(index));
}
void Portables::unfollowItem(HasShapeIndex index, ItemIndex item)
{
	m_area.getItems().m_follower.at(item).clear();
	m_leader.at(index).clear();
	m_area.getActors().move_updateActualSpeed(getLineLeader(index));
}
void Portables::unfollow(HasShapeIndex index)
{
	ActorOrItemIndex leader = m_leader.at(index);
	assert(leader.isLeading(m_area));
	if(leader.isActor())
		unfollowActor(index, leader.get());
	else
		unfollowItem(index, leader.get());
}
void Portables::unfollowIfAny(HasShapeIndex index)
{
	if(m_leader.at(index).exists())
		unfollow(index);
}
void Portables::leadAndFollowDisband(HasShapeIndex index)
{
	//Recurse to the end of the line and then unfollow forward to the front.
	ActorOrItemIndex follower = m_follower.at(index);
	if(follower.exists())
	{
		if(follower.isActor())
			m_area.getActors().leadAndFollowDisband(follower.get());
		else
			m_area.getItems().leadAndFollowDisband(follower.get());
	}
	if(m_leader.at(index).exists())
		unfollow(index);
	else
		assert(isActors);
}
bool Portables::isFollowing(HasShapeIndex index) const
{
	if(!m_follower.at(index).exists())
		return false;
	assert(m_follower.at(index).exists());
	return true;
}
bool Portables::isLeading(HasShapeIndex index) const
{
	return !m_leader.at(index).exists();
}
bool Portables::isLeadingActor(HasShapeIndex index, ActorIndex actor) const
{
	if(!isLeading(index))
		return false;
	const auto& leader = m_leader.at(index);
	return leader.isActor() && leader.get() == actor;
}
bool Portables::isLeadingItem(HasShapeIndex index, ItemIndex item) const
{
	if(!isLeading(index))
		return false;
	const auto& leader = m_leader.at(index);
	return leader.isItem() && leader.get() == item;
}
bool Portables::isLeadingPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem) const
{
	return m_follower.at(index) == actorOrItem;
}
Speed Portables::lead_getSpeed(HasShapeIndex index)
{
	assert(isActors);
	ActorIndex actorIndex = index;
	assert(!m_area.getActors().isFollowing(actorIndex));
	assert(m_area.getActors().isLeading(actorIndex));
	ActorOrItemIndex wrapped = getActorOrItemIndex(index);
	std::vector<ActorOrItemIndex> actorsAndItems;
	while(true)
	{
		actorsAndItems.push_back(wrapped);
		if(!wrapped.isLeading(m_area))
			break;
		wrapped = wrapped.getFollower(m_area);
	}
	return getMoveSpeedForGroupWithAddedMass(m_area, actorsAndItems, 0, 0);
}
ActorIndex Portables::getLineLeader(HasShapeIndex index)
{
	// Recursively traverse to the front of the line and return the leader.
	ActorOrItemIndex leader = m_leader.at(index);
	if(!leader.exists())
	{
		assert(m_follower.at(index).exists());
		assert(isActors);
		return index;
	}
	if(leader.isActor())
		return m_area.getActors().getLineLeader(leader.get());
	else
		return m_area.getItems().getLineLeader(leader.get());
}
// Class method.
Speed Portables::getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems)
{
	return getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, 0, 0);
}
Speed Portables::getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass, Mass addedDeadMass)
{
	Mass rollingMass = addedRollingMass;
	Mass deadMass = addedDeadMass;
	Mass carryMass = 0;
	Speed lowestMoveSpeed = 0;
	for(ActorOrItemIndex index : actorsAndItems)
	{
		if(index.isItem())
		{
			ItemIndex itemIndex = index.get();
			Mass mass = area.getItems().getMass(itemIndex);
			static const MoveType& roll = MoveType::byName("roll");
			if(area.getItems().getMoveType(itemIndex) == roll)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(index.isActor());
			ActorIndex actorIndex = index.get();
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
	float ratio = (float)carryMass / (float)totalMass;
	if(ratio < Config::minimumOverloadRatio)
		return 0;
	return std::ceil(lowestMoveSpeed * ratio * ratio);
}
void Portables::setCarrier(HasShapeIndex index, ActorOrItemIndex carrier)
{
	assert(!m_carrier.at(index).exists());
	m_carrier.at(index) = carrier;
}
void Portables::unsetCarrier(HasShapeIndex index, ActorOrItemIndex carrier)
{
	assert(m_carrier.at(index) == carrier);
	m_carrier.at(index).clear();
}
void Portables::updateIndexInCarrier(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	if(m_carrier.at(newIndex).isActor())
	{
		// Carrier is actor, either via canPickUp or equipmentSet.
		ActorIndex actor = m_carrier.at(newIndex).get();
		Actors& actors = m_area.getActors();
		if(isActors)
		{
			// actor is carrying actor
			assert(actors.canPickUp_isCarryingActor(oldIndex, actor));
			actors.canPickUp_updateActorIndex(actor, oldIndex, newIndex);
		}
		else
			if(actors.canPickUp_isCarryingItem(actor, oldIndex))
				actors.canPickUp_updateItemIndex(actor, oldIndex, newIndex);
			else
			{
				assert(actors.equipment_containsItem(actor, oldIndex));
				actors.equipment_updateItemIndex(actor, oldIndex, newIndex);
			}
	}
	else
	{
		// Carrier is item.
		ItemIndex item = m_carrier.at(newIndex).get();
		Items& items = m_area.getItems();
		assert(items.getItemType(item).internalVolume != 0);
		if(isActors)
			items.cargo_updateActorIndex(item, oldIndex, newIndex);
		else
			items.cargo_updateItemIndex(item, oldIndex, newIndex);
	}
}
void Portables::reservable_reserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables.at(index)->reserveFor(canReserve, quantity, std::move(callback));
}
void Portables::reservable_unreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity)
{
	m_reservables.at(index)->clearReservationFor(canReserve, quantity);
}
void Portables::reservable_unreserveFaction(HasShapeIndex index, const FactionId faction)
{
	m_reservables.at(index)->clearReservationsFor(faction);
}
void Portables::reservable_maybeUnreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity)
{
	m_reservables.at(index)->maybeClearReservationFor(canReserve, quantity);
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
void Portables::load(const Json& data)
{
	nlohmann::from_json(data, static_cast<Portables&>(*this));
	// We don't need to serialize OnDestroy because it only exists breifly between ThreadedTask read / write steps.
	// TODO: I feel like the above constraint about OnDestroy is violated somewhere and it's being used as a side channel reservation between steps. Make sure this is not the case or serialize it.
	m_follower = data["follower"].get<std::vector<ActorOrItemIndex>>();
	m_leader = data["leader"].get<std::vector<ActorOrItemIndex>>();
	m_moveType = data["moveType"].get<std::vector<const MoveType*>>();
	m_reservables.resize(m_moveType.size());
	for(const Json& pair : data["reservable"])
	{
		HasShapeIndex index = pair[0];
		m_reservables.at(index) = std::make_unique<Reservable>(pair[1]["maxReservations"].get<Quantity>());
		uintptr_t address;
		pair[1]["address"].get_to(address);
		m_area.m_simulation.getDeserializationMemo().m_reservables[address] = m_reservables.at(index).get();
	}
}
Json Portables::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output.update({
		{"follower", m_follower},
		{"leader", m_leader},
		{"moveType", m_moveType},
	});
	return output;
}
bool Portables::reservable_hasAnyReservations(HasShapeIndex index) const
{
	if(m_reservables.at(index) != nullptr)
		assert(m_reservables.at(index)->hasAnyReservations());
	return m_reservables.at(index) != nullptr;
}
bool Portables::reservable_exists(HasShapeIndex index, const FactionId faction) const
{
	if(m_reservables.at(index) == nullptr)
		return false;
	return m_reservables.at(index)->hasAnyReservationsWith(faction);
}
bool Portables::reservable_isFullyReserved(HasShapeIndex index, const FactionId faction) const
{
	return m_reservables.at(index)->isFullyReserved(faction);
}
Quantity Portables::reservable_getUnreservedCount(HasShapeIndex index, const FactionId faction) const
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
