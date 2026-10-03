#pragma once
#include "actorOrItemIndex.h"
#include "actors/actors.h"
#include "area/area.h"
#include "definitions/moveType.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "items/items.h"
#include "numericTypes/index.h"
#include "numericTypes/types.h"
#include "onDestroy.h"
#include "portables.h"
#include "reservable.h"
#include "reservable.hpp"
#include "simulation/simulation.h"
#include "space/space.h"
#include <cstdint>
#include <memory>

template<class Derived, class Index, class ReferenceIndex, bool isActors>
Portables<Derived, Index, ReferenceIndex, isActors>::Portables(Area& area) : HasShapes<Derived, Index>(area) { }
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::updateStoredIndicesPortables(Index oldIndex, Index newIndex)
{
	Actors& actors = this->m_area.getActors();
	Items& items = this->m_area.getItems();
	if(m_carrier[newIndex].exists())
		updateIndexInCarrier(oldIndex, newIndex);
	if(m_follower[newIndex].exists())
	{
		ActorOrItemIndex follower = m_follower[newIndex];
		if(follower.isActor())
		{
			assert(actors.getLeader(follower.getActor()).get() == oldIndex.get());
			actors.getLeader(follower.getActor()).updateIndex(newIndex);
		}
		else
		{
			assert(items.getLeader(follower.getItem()).get() == oldIndex.get());
			items.getLeader(follower.getItem()).updateIndex(newIndex);
		}
	}
	if(m_leader[newIndex].exists())
	{
		ActorOrItemIndex leader = m_leader[newIndex];
		if(leader.isActor())
		{
			ActorIndex actor = leader.getActor();
			assert(actors.getFollower(actor).get() == oldIndex.get());
			actors.getFollower(actor).updateIndex(newIndex);
		}
		else
		{
			ItemIndex item = leader.getItem();
			assert(items.getFollower(item).get() == oldIndex.get());
			items.getFollower(item).updateIndex(newIndex);
		}
	}
	ActorOrItemIndex oldIndexPolymorphic = getActorOrItemIndex(oldIndex);
	ActorOrItemIndex newIndexPolymorphic = getActorOrItemIndex(newIndex);
	if(m_isOnDeckOf[newIndex].exists())
	{
		ActorOrItemIndex isOnDeckOf = m_isOnDeckOf[newIndex];
		if(isOnDeckOf.isActor())
		{
			ActorIndex actor = isOnDeckOf.getActor();
			actors.onDeck_updateIndex(actor, oldIndexPolymorphic, newIndexPolymorphic);
		}
		if(isOnDeckOf.isItem())
		{
			ItemIndex item = isOnDeckOf.getItem();
			items.onDeck_updateIndex(item, oldIndexPolymorphic, newIndexPolymorphic);
		}
	}
	for(const ActorOrItemIndex& onDeck : m_onDeck[newIndex])
	{
		if(onDeck.isActor())
		{
			ActorIndex actor = onDeck.getActor();
			actors.onDeck_updateIsOnDeckOf(actor, newIndexPolymorphic);
		}
		if(onDeck.isItem())
		{
			ItemIndex item = onDeck.getItem();
			items.onDeck_updateIsOnDeckOf(item, newIndexPolymorphic);
		}
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::create(Index index, const MoveTypeId moveType, const ShapeId shape, FactionId faction, bool isStatic, const Quantity quantity)
{
	HasShapes<Derived, Index>::create(index, shape, faction, isStatic);
	m_moveType[index] = moveType;
	//TODO: leave as nullptr to start, create as needed.
	m_reservables[index] = std::make_unique<Reservable>(quantity);
	assert(m_destroy[index] == nullptr);
	assert(m_follower[index].empty());
	assert(m_leader[index].empty());
	assert(m_carrier[index].empty());
	assert(m_onDeck[index].empty());
	assert(m_isOnDeckOf[index].empty());
	assert(m_hasDecks[index].empty());
	assert(m_projectsOnDeck[index].empty());
	assert(m_floating[index].empty());
	// Corresponding remove in Actors::destroy and Items::destroy.
	m_referenceData.add(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onRemove(Index index)
{
	if(m_hasDecks[index].exists())
	{
		this->m_area.m_decks.unregisterDecks(this->m_area, m_hasDecks[index]);
		m_hasDecks[index].clear();
		m_projectsOnDeck[index].clear();
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::setFloating(Index index, const FluidTypeId fluidType, Distance depth)
{
	// Only dead actors float, otherwise they swim.
	if constexpr(isActors)
		assert(!this->m_area.getActors().isAlive(getActorOrItemIndex(index).getActor()));
	m_floating[index] = fluidType;
	MoveTypeId floatingMoveType = MoveType::getOrCreateForFloat(fluidType, depth);
	setMoveType(index, floatingMoveType);
	if constexpr(!isActors)
	{
		// TODO: update pilot speed.
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::unsetFloating(Index index)
{
	m_floating[index].clear();
	static_cast<Derived*>(this)->resetMoveType(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::setMoveType(Index index, const MoveTypeId moveType)
{
	assert(m_moveType[index] != moveType);
	m_moveType[index] = moveType;
	maybeFall(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Index Portables<Derived, Index, ReferenceIndex, isActors>::moveTo(Portables<Derived, Index, ReferenceIndex, isActors>& other, Index index)
{
	Index output = HasShapes<Derived, Index>::moveTo(other, index);
	// Reservations only exist in space.
	other.m_reservables.add();
	// OnDestroy is area specific, replace with a nullptr and implicitly trigger callbacks for the old one.
	other.m_destroy.add();
	// Leader and follower will be set by arriveInZone / exitZone.
	other.m_follower.add();
	other.m_leader.add();
	// Carrier will be set by the other side of the relationship.
	other.m_carrier.add();
	other.m_moveType.add(m_moveType[index]);
	// OnDeck and pilot will be set by arriveInZone / exitZone.
	other.m_onDeck.add();
	other.m_isOnDeckOf.add();
	other.m_hasDecks.add();
	// There can be no projects or fluids on deck without space.
	other.m_projectsOnDeck.add();
	other.m_cuboidsContainingFluidOnDeck.add();
	// Floating will be set by location_set.
	other.m_floating.add();
	other.m_referenceData.add(output);
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::log(Index index) const
{
	std::cout << ", moveType: " << MoveType::getName(m_moveType[index]);
	if(m_follower[index].exists())
		std::cout << ", leading: " << m_follower[index].toS();
	if(m_leader[index].exists())
		std::cout << ", following: " << m_leader[index].toS();
	if(m_carrier[index].exists())
		std::cout << ", carrier: " << m_carrier[index].toS();
	const Point3D location = getLocation(index);
	if(location.exists())
	{
		std::cout << ", location: " << location.toS();
		for(const Cuboid cuboid : this->m_occupied[index])
			for(const Point3D occupied : cuboid)
				if(occupied != location)
					std::cout << "-" << occupied.toS();
	}
	//if(!m_onDeck[index].empty())
		//std::cout << ", on deck " << m_onDeck[index].toS();
	if(m_isOnDeckOf[index].exists())
		std::cout << ", is on deck of " << m_isOnDeckOf[index].toS();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
ActorOrItemIndex Portables<Derived, Index, ReferenceIndex, isActors>::getActorOrItemIndex(Index index) const
{
	if constexpr(isActors)
		return ActorOrItemIndex::createForActor(ActorIndex::create(index.get()));
	else
		return ActorOrItemIndex::createForItem(ItemIndex::create(index.get()));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::followActor(Index index, ActorIndex actor)
{
	Actors& actors = this->m_area.getActors();
	assert(this->m_occupied[index].isTouching(actors.getOccupied(actor)));
	followActorAllowTeleport(index, actor);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::followActorAllowTeleport(Index index, ActorIndex actor)
{
	Actors& actors = this->m_area.getActors();
	assert(!isFollowing(index));
	assert(!isLeading(index));
	assert(!actors.isLeading(actor));
	if constexpr(isActors)
	{
		assert(!static_cast<Actors*>(this)->move_hasEvent(index));
		assert(!static_cast<Actors*>(this)->move_hasPathRequest(index));
	}
	// If following a pilot, follow the mount or vehicle instead.
	ActorOrItemIndex leaderPiloting = actors.getIsPiloting(actor);
	if(leaderPiloting.exists())
	{
		followPolymorphic(index, leaderPiloting);
		return;
	}
	// if piloting something then set that thing to follow rather then the pilot.
	if constexpr(isActors)
	{
		ActorOrItemIndex followerPiloting = actors.getIsPiloting(ActorIndex::create(index.get()));
		if(followerPiloting.exists())
		{
			followerPiloting.followActor(this->m_area, actor);
			return;
		}
	}
	m_leader[index] = ActorOrItemIndex::createForActor(actor);
	this->maybeUnsetStatic(index);
	actors.setFollower(actor, getActorOrItemIndex(index));
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::followItem(Index index, ItemIndex item)
{
	Actors& actors = this->m_area.getActors();
	Items& items = this->m_area.getItems();
	assert(!isFollowing(index));
	assert(!isLeading(index));
	assert(!items.isLeading(item));
	if constexpr(isActors)
	{
		assert(!static_cast<Actors*>(this)->move_hasEvent(index));
		assert(!static_cast<Actors*>(this)->move_hasPathRequest(index));
	}
	// Only follow items which already have leaders;
	assert(items.isFollowing(item));
	// if piloting something then set that thing to follow rather then the pilot.
	if constexpr(isActors)
	{
		ActorOrItemIndex followerPiloting = actors.getIsPiloting(ActorIndex::create(index.get()));
		if(followerPiloting.exists())
		{
			followerPiloting.followItem(this->m_area, item);
			return;
		}
	}
	m_leader[index] = ActorOrItemIndex::createForItem(item);
	this->maybeUnsetStatic(index);
	items.setFollower(item, getActorOrItemIndex(index));
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::followPolymorphic(Index index, ActorOrItemIndex actorOrItem)
{
	if(actorOrItem.isActor())
		followActor(index, ActorIndex::create(actorOrItem.get().get()));
	else
		followItem(index, ItemIndex::create(actorOrItem.get().get()));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::unfollowActor(Index index, ActorIndex actor)
{
	assert(!isLeading(index));
	assert(isFollowing(index));
	assert(m_leader[index] == actor.toActorOrItemIndex());
	if(!static_cast<Derived*>(this)->canMove(index))
		this->setStatic(index);
	Actors& actors = this->m_area.getActors();
	if(!actors.isFollowing(actor))
	{
		// Actor is line leader.
		assert(!actors.lineLead_getPath(actor).empty());
		actors.lineLead_clearPath(actor);
	}
	ActorIndex lineLeader = getLineLeader(index);
	actors.unsetFollower(actor, getActorOrItemIndex(index));
	m_leader[index].clear();
	this->m_area.getActors().move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::unfollowItem(Index index, ItemIndex item)
{
	assert(!isLeading(index));
	ActorIndex lineLeader = getLineLeader(index);
	m_leader[index].clear();
	if(!static_cast<Derived*>(this)->canMove(index))
		this->setStatic(index);
	Items& items = this->m_area.getItems();
	items.unsetFollower(item, getActorOrItemIndex(index));
	this->m_area.getActors().move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::unfollow(Index index)
{
	ActorOrItemIndex leader = m_leader[index];
	assert(leader.isLeading(this->m_area));
	if(leader.isActor())
		unfollowActor(index, leader.getActor());
	else
		unfollowItem(index, leader.getItem());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::unfollowIfAny(Index index)
{
	if(m_leader[index].exists())
		unfollow(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::leadAndFollowDisband(Index index)
{
	assert(isFollowing(index) || isLeading(index));
	ActorOrItemIndex follower = getActorOrItemIndex(index);
	// Go to the back of the line.
	while(follower.isLeading(this->m_area))
		follower = follower.getFollower(this->m_area);
	// Iterate to the second from front unfollowing all followers.
	// TODO: This will cause a redundant updateActualSpeed for each follower.
	ActorOrItemIndex leader;
	while(follower.isFollowing(this->m_area))
	{
		leader = follower.getLeader(this->m_area);
		follower.unfollow(this->m_area);
		follower = leader;
	}
	this->m_area.getActors().lineLead_clearPath(leader.getActor());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::maybeLeadAndFollowDisband(Index index)
{
	if(isFollowing(index) || isLeading(index))
		leadAndFollowDisband(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::isFollowing(Index index) const
{
	return m_leader[index].exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeading(Index index) const
{
	return m_follower[index].exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeadingActor(Index index, ActorIndex actor) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isActor() && follower.getActor() == actor;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeadingItem(Index index, ItemIndex item) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isItem() && follower.getItem() == item;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeadingPolymorphic(Index index, ActorOrItemIndex actorOrItem) const
{
	return m_follower[index] == actorOrItem;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
MapWithCuboidKeys<CollisionVolume> Portables<Derived, Index, ReferenceIndex, isActors>::getOccupiedCombinedWithVolumes(Index index) const
{
	const MapWithCuboidKeys<CollisionVolume>& occupied = this->m_occupiedWithVolume[index];
	MapWithCuboidKeys<CollisionVolume> output;
	int toReserve = occupied.size();
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		toReserve += Shape::getCuboidsCount(onDeck.getShape(this->m_area));
	output.reserve(toReserve);
	output = occupied;
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		for(const auto& pair : onDeck.getOccupiedWithVolume(this->m_area))
			output.insertOrMerge(pair);
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
CuboidSet Portables<Derived, Index, ReferenceIndex, isActors>::getOccupiedCombined(Index index) const
{
	const CuboidSet& occupied = this->m_occupied[index];
	int toReserve = occupied.size();
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		toReserve += Shape::getCuboidsCount(onDeck.getShape(this->m_area));
	CuboidSet output;
	output.reserve(toReserve);
	output.maybeAddAll(occupied);
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		output.maybeAddAll(onDeck.getOccupied(this->m_area));
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Distance Portables<Derived, Index, ReferenceIndex, isActors>::floatsInAtDepth(Index index, const FluidTypeId fluidType) const
{
	Mass mass = static_cast<const Derived*>(this)->getMass(index);
	CollisionVolume displacement = CollisionVolume::create(0);
	Distance output = Distance::create(0);
	const Density fluidDensity = FluidType::getDensity(fluidType);
	const ShapeId shape = HasShapes<Derived, Index>::getShape(index);
	Distance shapeHeight = Distance::create(Shape::getZSize(shape).get());
	OffsetCuboidSet previousLevel;
	OffsetCuboidSet nextLevel;
	const Cuboid boundry = this->m_area.getSpace().boundry();
	while(fluidDensity < mass / displacement.toVolume())
	{
		MapWithOffsetCuboidKeys<CollisionVolume> thisLevel = Shape::getCuboidsWithVolumeByZLevel(shape, output);
		if(thisLevel.size() < previousLevel.volume() || shapeHeight < output)
		{
			// Hull ends here.
			return Distance::null();
		}
		// Anything directly above a recorded point is assumed to be eiter part of or contained within the hull.
		displacement += Config::maxPointVolume * nextLevel.volume();
		// Use previous level to ensure we don't record the same point twice.
		previousLevel.swap(nextLevel);
		nextLevel.clear();
		for(const std::pair<OffsetCuboid, CollisionVolume>& cuboidAndVolume : thisLevel)
		{
			if(nextLevel.contains(cuboidAndVolume.first))
				continue;
			OffsetCuboid above = cuboidAndVolume.first.above();
			assert(boundry.contains(above));
			nextLevel.maybeAdd(above);
			displacement += cuboidAndVolume.second;
		}
		++output;
	}
	// Use depth 0 to mean the topmost block which contains the fluid.
	return output - 1;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::canFloatAt(Index index, const Point3D point, const Facing4 facing) const
{
	return getFluidTypeCanFloatInAt(index, point, facing).exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
FluidTypeId Portables<Derived, Index, ReferenceIndex, isActors>::getFluidTypeCanFloatInAt(Index index, const Point3D point, const Facing4 facing) const
{
	const Space& space = this->m_area.getSpace();
	for(const FluidData& fluidData : space.fluid_getAll(point))
		if(canFloatAtInFluidTypeWithFacing(index, point, fluidData.type, facing))
			return fluidData.type;
	return FluidTypeId::null();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::canFloatAtInFluidTypeWithFacing(Index index, const Point3D point, const FluidTypeId fluidType, const Facing4 facing) const
{
	const Distance floatDepth = floatsInAtDepth(index, fluidType);
	if(floatDepth.empty())
		// Cannot float in this fluid at any depth.
		return false;
	const Space& space = this->m_area.getSpace();
	FluidGroup* fluidGroup = space.fluid_getGroup(point, fluidType);
	if(fluidGroup->m_stable)
		return fluidGroup->m_highZ - floatDepth >= point.z();
	else
		return space.fluid_shapeIsMostlySurroundedByFluidOfTypeAtDistanceAboveLocationWithFacing(getShape(index), fluidType, floatDepth, point, facing);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Speed Portables<Derived, Index, ReferenceIndex, isActors>::lead_getSpeed(Index index) const
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	const Actors& actors = this->m_area.getActors();
	ActorOrItemIndex wrapped = getActorOrItemIndex(index);
	if constexpr(isActors)
	{
 		ActorOrItemIndex isPiloting = actors.getIsPiloting(static_cast<const ActorIndex>(index));
		if(isPiloting.exists())
			wrapped = isPiloting;
	}
	std::vector<ActorOrItemIndex> actorsAndItems;
	while(wrapped.exists())
	{
		actorsAndItems.push_back(wrapped);
		if(!wrapped.isLeading(this->m_area))
			break;
		wrapped = wrapped.getFollower(this->m_area);
	}
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(this->m_area, actorsAndItems, Mass::create(0), Mass::create(0), Mass::create(0));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
ActorIndex Portables<Derived, Index, ReferenceIndex, isActors>::getLineLeader(Index index) const
{
	// Recursively traverse to the front of the line and return the leader.
	const Items& items = this->m_area.getItems();
	ActorOrItemIndex leader = m_leader[index];
	assert(getActorOrItemIndex(index) != leader);
	if(leader.empty())
	{
		assert(m_follower[index].exists());
		if constexpr(!isActors)
		{
			// If the leader is an item then return it's pilot.
			ActorIndex pilot = items.pilot_get(leader.getItem());
			assert(pilot.exists());
			return pilot;
		}
		return ActorIndex::create(index.get());
	}
	if(leader.isFollowing(this->m_area))
		assert(leader.getLeader(this->m_area) != getActorOrItemIndex(index));
	assert(leader.isLeading(this->m_area));
	assert(leader.getFollower(this->m_area) == getActorOrItemIndex(index));
	if(leader.isActor())
		return this->m_area.getActors().getLineLeader(leader.getActor());
	else
		return this->m_area.getItems().getLineLeader(leader.getItem());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::movingIndependently(Index index) const
{
	return !(
		isFollowing(index) ||
		onDeck_isOnDeck(index) ||
		hasCarrier(index)
	);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::hasCarrier(Index index) const
{
	return m_carrier[index].exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::setCarrier(Index index, ActorOrItemIndex carrier)
{
	assert(!m_carrier[index].exists());
	m_carrier[index] = carrier;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::maybeSetCarrier(Index index, ActorOrItemIndex carrier)
{
	if(m_carrier[index] == carrier)
		return;
	setCarrier(index, carrier);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::unsetCarrier(Index index, [[maybe_unused]] ActorOrItemIndex carrier)
{
	assert(m_carrier[index] == carrier);
	m_carrier[index].clear();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::maybeFall(Index index)
{
	if(MoveType::getFly(m_moveType[index]))
		return;
	Space& space = this->m_area.getSpace();
	const Point3D location = getLocation(index);
	if(location.z() == 0)
		return;
	const Point3D below = location.below();
	const ShapeId shape = getShape(index);
	const CuboidSet& occupied = this->m_occupied[index];
	const Facing4& facing = getFacing(index);
	if(space.shape_canStandIn(location))
		return;
	if(space.shape_canFitEverOrCurrentlyDynamic(below, shape, facing, occupied) && !canFloatAt(index, location, facing))
		fall(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::fall(Index index)
{
	Space& space = this->m_area.getSpace();
	const ShapeId shape = getShape(index);
	const Facing4& facing = getFacing(index);
	Point3D location = getLocation(index);
	assert(location.exists());
	assert(location.z() != 0);
	assert(!canFloatAt(index, location, facing));
	assert(!isFloating(index));
	assert(location.exists());
	Distance distance = Distance::create(0);
	Point3D next;
	const auto& occupied = this->getOccupied(index);
	while(location.z() != 0)
	{
		next = location.below();
		if(space.shape_canFitEverOrCurrentlyDynamic(next, shape, facing, occupied) && !canFloatAt(index, location, facing))
		{
			location = next;
			++distance;
		}
		else
			break;
	}
	assert(distance != 0);
	auto pointsBelowEndPosition = space.shape_getBelowPointsWithFacing(location, shape, facing);
	const auto [materialType, hardness] = space.solid_getHardest(pointsBelowEndPosition);
	static_cast<Derived*>(this)->location_set(index, location, facing);
	static_cast<Derived*>(this)->takeFallDamage(index, distance, materialType);
	//TODO: dig out / destruct below impact.
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Mass Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_getMass(Index index) const
{
	Mass output = Mass::create(0);
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		output += onDeck.getMass(this->m_area);
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onSetLocation(Index index, const Point3D previousLocation, const Facing4 previousFacing)
{
	Space& space = this->m_area.getSpace();
	const Point3D newLocation = getLocation(index);
	assert(newLocation.exists());
	const Facing4 newFacing = getFacing(index);
	if(space.isExposedToSky(newLocation))
		static_cast<Derived*>(this)->setOnSurface(index, true);
	else
		static_cast<Derived*>(this)->setOnSurface(index, false);
	// Move decks attached to this portable.
	const DeckId deckId = m_hasDecks[index];
	if(deckId.exists())
	{
		assert(previousLocation.exists());
		Offset3D offset = previousLocation.offsetTo(getLocation(index));
		this->m_area.m_decks.shift(this->m_area, deckId, offset, Distance::create(1), previousLocation, previousFacing, newFacing);
	}
	else
	{
		// TODO: move this to onInsertIntoPoints method.
		const OffsetCuboidSet& deckOffsets = static_cast<Derived*>(this)->getDeckOffsets(index);
		if(!deckOffsets.empty())
		{
			// Create decks for newly added.
			CuboidSet decks = CuboidSet::create(space.offsetBoundry(), newLocation, newFacing, deckOffsets);
			m_hasDecks[index] = this->m_area.m_decks.registerDecks(this->m_area, decks, getActorOrItemIndex(index));
		}
	}
	// Update which deck this portable is on.
	DeckId onDeckOf = this->m_area.m_decks.queryDeckId(getLocation(index));
	DeckId onDeckOfPrevious = previousLocation.exists() ? this->m_area.m_decks.queryDeckId(previousLocation) : DeckId::null();
	if(onDeckOfPrevious == deckId)
		onDeckOfPrevious.clear();
	if(onDeckOf != onDeckOfPrevious)
	{
		if(onDeckOf == DeckId::null())
			onDeck_clear(index);
		else
		{
			ActorOrItemIndex onDeckOfIndex = this->m_area.m_decks.getForId(onDeckOf);
			onDeck_set(index, onDeckOfIndex);
		}
	}
	// Set floating.
	// TODO: opitmize this.
	if constexpr(!isActors)
	{
		const FluidTypeId fluidType = getFluidTypeCanFloatInAt(index, newLocation, newFacing);
		if(fluidType.exists())
		{
			if(!isFloating(index))
			{
				//TODO:(optimization) This call is redundant.
				Distance depth = floatsInAtDepth(index, fluidType);
				setFloating(index, fluidType, depth);
			}
		}
		else if(isFloating(index))
			setNotFloating(index);
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::updateIndexInCarrier(Index oldIndex, Index newIndex)
{
	if(m_carrier[newIndex].isActor())
	{
		// Carrier is actor, either via canPickUp or equipmentSet.
		ActorIndex actor = ActorIndex::cast(m_carrier[newIndex].get());
		Actors& actors = this->m_area.getActors();
		if constexpr(isActors)
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
		Items& items = this->m_area.getItems();
		assert(ItemType::getInternalVolume(items.getItemType(item)) != 0);
		if constexpr(isActors)
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
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_reserve(Index index, CanReserve& canReserve, const Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->reserveFor(canReserve, quantity, std::move(callback));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_unreserve(Index index, CanReserve& canReserve, const Quantity quantity)
{
	m_reservables[index]->clearReservationFor(canReserve, quantity);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_unreserveFaction(Index index, FactionId faction)
{
	m_reservables[index]->clearReservationsFor(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_maybeUnreserve(Index index, CanReserve& canReserve, const Quantity quantity)
{
	m_reservables[index]->maybeClearReservationFor(canReserve, quantity);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_unreserveAll(Index index)
{
	m_reservables[index]->clearAll();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_setDishonorCallback(Index index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->setDishonorCallbackFor(canReserve, std::move(callback));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_merge(Index index, Reservable& other)
{
	m_reservables[index]->merge(other);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::load(const Json& data)
{
	nlohmann::from_json(data, static_cast<HasShapes<Derived, Index>&>(*this));
	data["moveType"].get_to(m_moveType);
	data["referenceData"].get_to(m_referenceData);
	data["follower"].get_to(m_follower);
	data["leader"].get_to(m_leader);
	data["onDeck"].get_to(m_onDeck);
	data["isOnDeckOf"].get_to(m_isOnDeckOf);
	m_reservables.resize(m_moveType.size());
	DeserializationMemo& deserializationMemo = this->m_area.m_simulation.getDeserializationMemo();
	assert(data["reservable"].type() == Json::value_t::object);
	for(auto iter = data["reservable"].begin(); iter != data["reservable"].end(); ++iter)
	{
		Index index = Index::create(std::stoi(iter.key()));
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
		Index index = Index::create(std::stoi(iter.key()));
		m_destroy[index] = std::make_unique<OnDestroy>(iter.value(), deserializationMemo, ActorOrItemReference(getReference(index)));
		uintptr_t address;
		iter.value().get_to(address);
		deserializationMemo.m_reservables[address] = m_reservables[index].get();
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Json Portables<Derived, Index, ReferenceIndex, isActors>::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output.update({
		{"follower", m_follower},
		{"leader", m_leader},
		{"onDestroy", Json::object()},
		{"reservable", Json::object()},
		{"moveType", m_moveType},
		{"referenceData", m_referenceData},
		{"onDeck", m_onDeck},
		{"isOnDeckOf", m_isOnDeckOf},
		{"hasDecks", m_hasDecks}
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
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_hasAnyReservations(Index index) const
{
	return m_reservables[index]->hasAnyReservations();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_exists(Index index, FactionId faction) const
{
	return m_reservables[index]->hasAnyReservationsWith(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_existsFor(Index index, const CanReserve& canReserve) const
{
	return m_reservables[index]->hasAnyReservationsFor(canReserve);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_isFullyReserved(Index index, FactionId faction) const
{
	return m_reservables[index]->isFullyReserved(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Quantity Portables<Derived, Index, ReferenceIndex, isActors>::reservable_getUnreservedCount(Index index, FactionId faction) const
{
	return m_reservables[index]->getUnreservedCount(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_subscribe(Index index, HasOnDestroySubscriptions& hasSubscriptions)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>(getReference(index));
	hasSubscriptions.subscribe(*m_destroy[index].get());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_subscribeThreadSafe(Index index, HasOnDestroySubscriptions& hasSubscriptions)
{
	std::lock_guard<std::mutex> lock(HasOnDestroySubscriptions::m_mutex);
	onDestroy_subscribe(index, hasSubscriptions);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_unsubscribe(Index index, HasOnDestroySubscriptions& hasSubscriptions)
{
	m_destroy[index]->unsubscribe(hasSubscriptions);
	if(m_destroy[index]->empty())
		m_destroy[index] = nullptr;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_unsubscribeAll(Index index)
{
	m_destroy[index]->unsubscribeAll();
	m_destroy[index] = nullptr;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_merge(Index index, OnDestroy& other)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>(getReference(index));
	m_destroy[index]->merge(other);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
DeckId Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_createDecks(Index index, const CuboidSet& cuboidSet)
{
	DeckId output = this->m_area.m_decks.registerDecks(this->m_area, cuboidSet, ActorOrItemIndex::create(index));
	m_hasDecks[index] = output;
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_destroyDecks(Index index)
{
	assert(m_hasDecks[index].exists());
	this->m_area.m_decks.unregisterDecks(this->m_area, m_hasDecks[index]);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_clear(Index index)
{
	ActorOrItemIndex onDeckOf = m_isOnDeckOf[index];
	assert(onDeckOf.isItem());
	ItemIndex onDeckOfItem = onDeckOf.getItem();
	Items& items = this->m_area.getItems();
	items.onDeck_removeFromOnDeck(onDeckOfItem, getActorOrItemIndex(index));
	ActorIndex pilot = items.pilot_get(onDeckOfItem);
	if(pilot.exists())
		this->m_area.getActors().move_setMoveSpeedActual(pilot, items.vehicle_getSpeed(onDeckOfItem));
	m_isOnDeckOf[index].clear();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_set(Index index, ActorOrItemIndex onDeckOf)
{
	if(m_isOnDeckOf[index].exists())
		onDeck_clear(index);
	m_isOnDeckOf[index] = onDeckOf;
	assert(onDeckOf.isItem());
	ItemIndex onDeckOfItem = onDeckOf.getItem();
	Items& items = this->m_area.getItems();
	items.onDeck_insertIntoOnDeck(onDeckOfItem, getActorOrItemIndex(index));
	ActorIndex pilot = items.pilot_get(onDeckOfItem);
	if(pilot.exists())
		this->m_area.getActors().move_setMoveSpeedActual(pilot, items.vehicle_getSpeed(onDeckOfItem));
}