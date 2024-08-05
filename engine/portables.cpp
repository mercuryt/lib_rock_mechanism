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

Portables::Portables(Area& area) : HasShapes(area) { }
void Portables::resize(HasShapeIndex newSize)
{
	HasShapes::resize(newSize);
	m_moveType.resize(newSize.get());
	m_destroy.resize(newSize.get());
	m_reservables.resize(newSize.get());
	m_follower.resize(newSize.get());
	m_leader.resize(newSize.get());
	m_carrier.resize(newSize.get());
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
		return ActorOrItemIndex::createForActor(ActorIndex::cast(index));
	else
		return ActorOrItemIndex::createForItem(ItemIndex::cast(index));
}
void Portables::followActor(HasShapeIndex index, ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	assert(!m_leader.at(index).exists());
	m_leader.at(index) = ActorOrItemIndex::createForActor(actor);
	assert(!actors.m_follower.at(index).exists());
	actors.m_follower.at(index) = getActorOrItemIndex(index);
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
	actors.move_appendToLinePath(lineLeader, m_location.at(index));
}
void Portables::followItem(HasShapeIndex index, ItemIndex item)
{
	Actors& actors = m_area.getActors();
	assert(!m_leader.at(index).exists());
	assert(!m_leader.at(index).exists());
	m_leader.at(index) = ActorOrItemIndex::createForItem(item);
	assert(!m_area.getItems().m_follower.at(index).exists());
	m_area.getItems().m_follower.at(index) = getActorOrItemIndex(index);
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
	actors.move_appendToLinePath(lineLeader, m_location.at(index));
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
	assert(isFollowing(index) == actor);
	Actors& actors = m_area.getActors();
	actors.m_follower.at(actor).clear();
	m_leader.at(index).clear();
	if(!isFollowing(actor))
	{
		// Actor is line leader.
		assert(!actors.lineLead_getPath(actor).empty());
		actors.lineLead_clearPath(actor);
	}
	ActorIndex lineLeader = getLineLeader(index);
	m_area.getActors().move_updateActualSpeed(lineLeader);
}
void Portables::unfollowItem(HasShapeIndex index, ItemIndex item)
{
	assert(!isLeading(index));
	m_area.getItems().m_follower.at(item).clear();
	m_leader.at(index).clear();
	m_area.getActors().move_updateActualSpeed(getLineLeader(index));
}
void Portables::unfollow(HasShapeIndex index)
{
	ActorOrItemIndex leader = m_leader.at(index);
	assert(leader.isLeading(m_area));
	if(leader.isActor())
		unfollowActor(index, ActorIndex::cast(leader.get()));
	else
		unfollowItem(index, ItemIndex::cast(leader.get()));
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
	ActorIndex actorIndex = ActorIndex::cast(index);
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
	return getMoveSpeedForGroupWithAddedMass(m_area, actorsAndItems, Mass::create(0), Mass::create(0));
}
ActorIndex Portables::getLineLeader(HasShapeIndex index)
{
	// Recursively traverse to the front of the line and return the leader.
	ActorOrItemIndex leader = m_leader.at(index);
	if(!leader.exists())
	{
		assert(m_follower.at(index).exists());
		assert(isActors);
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
			static const MoveType& roll = MoveType::byName("roll");
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
		ActorIndex actor = ActorIndex::cast(m_carrier.at(newIndex).get());
		Actors& actors = m_area.getActors();
		if(isActors)
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
			else
			{
				assert(actors.equipment_containsItem(actor, oi));
				actors.equipment_updateItemIndex(actor, oi, ni);
			}
		}
	}
	else
	{
		// Carrier is item.
		ItemIndex item = ItemIndex::cast(m_carrier.at(newIndex).get());
		Items& items = m_area.getItems();
		assert(items.getItemType(item).internalVolume != 0);
		if(isActors)
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
	m_reservables.at(index)->merge(other);
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
		m_reservables.at(index) = std::make_unique<Reservable>(pair[1]["maxReservations"].get<Quantity>());
		uintptr_t address;
		pair[1]["address"].get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables.at(index).get();
	}
	m_destroy.resize(m_moveType.size());
	for(const Json& pair : data["onDestroy"])
	{
		HasShapeIndex index = pair[0];
		m_destroy.at(index) = std::make_unique<OnDestroy>(pair[1], deserializationMemo);
		uintptr_t address;
		pair[1]["address"].get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables.at(index).get();
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
		if(m_destroy.at(i) != nullptr)
			output[i.get()] = *m_destroy.at(i).get();
		if(m_reservables.at(i) != nullptr)
			output[i.get()] = reinterpret_cast<uintptr_t>(m_reservables.at(i).get());
	}
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
	hasSubscriptions.subscribe(*m_destroy.at(index).get());
}
void Portables::onDestroy_subscribeThreadSafe(HasShapeIndex index, HasOnDestroySubscriptions& hasSubscriptions)
{
	std::lock_guard<std::mutex> lock(HasOnDestroySubscriptions::m_mutex);
	onDestroy_subscribe(index, hasSubscriptions);
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
	m_destroy.at(index)->merge(other);
}
