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
void Portables<Derived, Index, ReferenceIndex, isActors>::updateStoredIndicesPortables(const Index& oldIndex, const Index& newIndex)
{
	Actors& actors = getActors();
	Items& items = getItems();
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
			assert(getItems().getLeader(follower.getItem()).get() == oldIndex.get());
			getItems().getLeader(follower.getItem()).updateIndex(newIndex);
		}
	}
	if(m_leader[newIndex].exists())
	{
		ActorOrItemIndex leader = m_leader[newIndex];
		if(leader.isActor())
		{
			const ActorIndex& actor = leader.getActor();
			assert(actors.getFollower(actor).get() == oldIndex.get());
			actors.getFollower(actor).updateIndex(newIndex);
		}
		else
		{
			const ItemIndex& item = leader.getItem();
			assert(items.getFollower(item).get() == oldIndex.get());
			items.getFollower(item).updateIndex(newIndex);
		}
	}
	ActorOrItemIndex oldIndexPolymorphic = getActorOrItemIndex(oldIndex);
	ActorOrItemIndex newIndexPolymorphic = getActorOrItemIndex(newIndex);
	if(m_isOnDeckOf[newIndex].exists())
	{
		const ActorOrItemIndex& isOnDeckOf = m_isOnDeckOf[newIndex];
		if(isOnDeckOf.isActor())
		{
			const ActorIndex& actor = isOnDeckOf.getActor();
			actors.onDeck_updateIndex(actor, oldIndexPolymorphic, newIndexPolymorphic);
		}
		if(isOnDeckOf.isItem())
		{
			const ItemIndex& item = isOnDeckOf.getItem();
			items.onDeck_updateIndex(item, oldIndexPolymorphic, newIndexPolymorphic);
		}
	}
	for(const ActorOrItemIndex& onDeck : m_onDeck[newIndex])
	{
		if(onDeck.isActor())
		{
			const ActorIndex& actor = onDeck.getActor();
			actors.onDeck_updateIsOnDeckOf(actor, newIndexPolymorphic);
		}
		if(onDeck.isItem())
		{
			const ItemIndex& item = onDeck.getItem();
			items.onDeck_updateIsOnDeckOf(item, newIndexPolymorphic);
		}
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::create(const Index& index, const MoveTypeId& moveType, const ShapeId& shape, const FactionId& faction, bool isStatic, const Quantity& quantity)
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
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::onRemove(const Index& index)
{
	if(m_hasDecks[index].exists())
	{
		Area& area = getArea();
		area.m_decks.unregisterDecks(area, m_hasDecks[index]);
		m_hasDecks[index].clear();
		m_projectsOnDeck[index].clear();
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::setFloating(const Index& index, const FluidTypeId& fluidType)
{
	// Only dead actors float, otherwise they swim.
	if constexpr(isActors)
		assert(!getActors().isAlive(getActorOrItemIndex(index).getActor()));
	m_floating[index] = fluidType;
	static const MoveTypeId& floatingMoveType = MoveType::byName("floating");
	setMoveType(index, floatingMoveType);
	if constexpr(!isActors)
	{
		// TODO: update pilot speed.
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::unsetFloating(const Index& index)
{
	m_floating[index].clear();
	static_cast<Derived*>(this)->resetMoveType(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::setMoveType(const Index& index, const MoveTypeId& moveType)
{
	assert(m_moveType[index] != moveType);
	m_moveType[index] = moveType;
	getArea().m_hasTerrainFacades.maybeRegisterMoveType(moveType);
	maybeFall(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::log(const Index& index) const
{
	std::cout << ", moveType: " << MoveType::getName(m_moveType[index]);
	if(m_follower[index].exists())
		std::cout << ", leading: " << m_follower[index].toString();
	if(m_leader[index].exists())
		std::cout << ", following: " << m_leader[index].toString();
	if(m_carrier[index].exists())
		std::cout << ", carrier: " << m_carrier[index].toString();
	const Point3D& location = getLocation(index);
	if(location.exists())
	{
		std::cout << ", location: " << location.toString();
		for(const Cuboid& cuboid : this->m_occupied[index])
			for(const Point3D& occupied : cuboid)
				if(occupied != location)
					std::cout << "-" << occupied.toString();
	}
	//if(!m_onDeck[index].empty())
		//std::cout << ", on deck " << m_onDeck[index].toString();
	if(m_isOnDeckOf[index].exists())
		std::cout << ", is on deck of " << m_isOnDeckOf[index].toString();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>ActorOrItemIndex Portables<Derived, Index, ReferenceIndex, isActors>::getActorOrItemIndex(const Index& index)
{
	if constexpr(isActors)
		return ActorOrItemIndex::createForActor(ActorIndex::create(index.get()));
	else
		return ActorOrItemIndex::createForItem(ItemIndex::create(index.get()));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::followActor(const Index& index, const ActorIndex& actor)
{
	Actors& actors = getActors();
	assert(this->m_occupied[index].isTouching(actors.getOccupied(actor)));
	followActorAllowTeleport(index, actor);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::followActorAllowTeleport(const Index& index, const ActorIndex& actor)
{
	Actors& actors = getActors();
	assert(!isFollowing(index));
	assert(!isLeading(index));
	assert(!actors.isLeading(actor));
	if constexpr(isActors)
	{
		assert(!static_cast<Actors*>(this)->move_hasEvent(index));
		assert(!static_cast<Actors*>(this)->move_hasPathRequest(index));
	}
	// If following a pilot, follow the mount or vehicle instead.
	const ActorOrItemIndex& leaderPiloting = actors.getIsPiloting(actor);
	if(leaderPiloting.exists())
	{
		followPolymorphic(index, leaderPiloting);
		return;
	}
	// if piloting something then set that thing to follow rather then the pilot.
	if constexpr(isActors)
	{
		const ActorOrItemIndex& followerPiloting =  actors.getIsPiloting(ActorIndex::create(index.get()));
		if(followerPiloting.exists())
		{
			followerPiloting.followActor(getArea(), actor);
			return;
		}
	}
	m_leader[index] = ActorOrItemIndex::createForActor(actor);
	this->maybeUnsetStatic(index);
	actors.setFollower(actor, getActorOrItemIndex(index));
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::followItem(const Index& index, const ItemIndex& item)
{
	Actors& actors = getActors();
	Items& items = getItems();
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
		const ActorOrItemIndex& followerPiloting = actors.getIsPiloting(ActorIndex::create(index.get()));
		if(followerPiloting.exists())
		{
			followerPiloting.followItem(getArea(), item);
			return;
		}
	}
	m_leader[index] = ActorOrItemIndex::createForItem(item);
	this->maybeUnsetStatic(index);
	items.getFollower(item) = getActorOrItemIndex(index);
	ActorIndex lineLeader = getLineLeader(index);
	actors.move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::followPolymorphic(const Index& index, const ActorOrItemIndex& actorOrItem)
{
	if(actorOrItem.isActor())
		followActor(index, ActorIndex::create(actorOrItem.get().get()));
	else
		followItem(index, ItemIndex::create(actorOrItem.get().get()));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::unfollowActor(const Index& index, const ActorIndex& actor)
{
	assert(!isLeading(index));
	assert(isFollowing(index));
	assert(m_leader[index] == actor.toActorOrItemIndex());
	if(!static_cast<Derived*>(this)->canMove(index))
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
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::unfollowItem(const Index& index, const ItemIndex& item)
{
	assert(!isLeading(index));
	ActorIndex lineLeader = getLineLeader(index);
	m_leader[index].clear();
	if(!static_cast<Derived*>(this)->canMove(index))
		this->setStatic(index);
	Items& items = getItems();
	items.unsetFollower(item, getActorOrItemIndex(index));
	getActors().move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::unfollow(const Index& index)
{
	ActorOrItemIndex leader = m_leader[index];
	assert(leader.isLeading(getArea()));
	if(leader.isActor())
		unfollowActor(index, leader.getActor());
	else
		unfollowItem(index, leader.getItem());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::unfollowIfAny(const Index& index)
{
	if(m_leader[index].exists())
		unfollow(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::leadAndFollowDisband(const Index& index)
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
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::maybeLeadAndFollowDisband(const Index& index)
{
	if(isFollowing(index) || isLeading(index))
		leadAndFollowDisband(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>bool Portables<Derived, Index, ReferenceIndex, isActors>::isFollowing(const Index& index) const
{
	return m_leader[index].exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeading(const Index& index) const
{
	return m_follower[index].exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeadingActor(const Index& index, const ActorIndex& actor) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isActor() && follower.getActor() == actor;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeadingItem(const Index& index, const ItemIndex& item) const
{
	if(!isLeading(index))
		return false;
	const auto& follower = m_follower[index];
	return follower.isItem() && follower.getItem() == item;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>bool Portables<Derived, Index, ReferenceIndex, isActors>::isLeadingPolymorphic(const Index& index, const ActorOrItemIndex& actorOrItem) const
{
	return m_follower[index] == actorOrItem;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
MapWithCuboidKeys<CollisionVolume> Portables<Derived, Index, ReferenceIndex, isActors>::getOccupiedCombinedWithVolumes(const Index& index) const
{
	const MapWithCuboidKeys<CollisionVolume>& occupied = this->m_occupiedWithVolume[index];
	MapWithCuboidKeys<CollisionVolume> output;
	const Area& area = getArea();
	int toReserve = occupied.size();
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		toReserve += Shape::getCuboidsCount(onDeck.getShape(area));
	output.reserve(toReserve);
	output = occupied;
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		for(const auto& pair : onDeck.getOccupiedWithVolume(area))
			output.insertOrMerge(pair);
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
CuboidSet Portables<Derived, Index, ReferenceIndex, isActors>::getOccupiedCombined(const Index& index) const
{
	const CuboidSet& occupied = this->m_occupied[index];
	const Area& area = getArea();
	int toReserve = occupied.size();
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		toReserve += Shape::getCuboidsCount(onDeck.getShape(area));
	CuboidSet output;
	output.reserve(toReserve);
	output.maybeAddAll(occupied);
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		output.maybeAddAll(onDeck.getOccupied(area));
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Distance Portables<Derived, Index, ReferenceIndex, isActors>::floatsInAtDepth(const Index& index, const FluidTypeId& fluidType) const
{
	const Mass mass = static_cast<const Derived*>(this)->getMass(index);
	CollisionVolume displacement = CollisionVolume::create(0);
	Distance output = Distance::create(0);
	const Density& fluidDensity = FluidType::getDensity(fluidType);
	const ShapeId& shape = HasShapes<Derived, Index>::getShape(index);
	Distance shapeHeight = Distance::create(Shape::getZSize(shape).get());
	OffsetCuboidSet previousLevel;
	OffsetCuboidSet nextLevel;
	const Cuboid boundry = getArea().getSpace().boundry();
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
bool Portables<Derived, Index, ReferenceIndex, isActors>::canFloatAt(const Index& index, const Point3D& point, const Facing4& facing) const
{
	return getFluidTypeCanFloatInAt(index, point, facing).exists();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
FluidTypeId Portables<Derived, Index, ReferenceIndex, isActors>::getFluidTypeCanFloatInAt(const Index& index, const Point3D& point, const Facing4& facing) const
{
	const Space& space = getArea().getSpace();
	for(const FluidData& fluidData : space.fluid_getAll(point))
		if(canFloatAtInFluidTypeWithFacing(index, point, fluidData.type, facing))
			return fluidData.type;
	return FluidTypeId::null();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::canFloatAtInFluidTypeWithFacing(const Index& index, const Point3D& point, const FluidTypeId& fluidType, const Facing4& facing) const
{
	const Distance& floatDepth = floatsInAtDepth(index, fluidType);
	if(floatDepth.empty())
		// Cannot float in this fluid at any depth.
		return false;
	const Space& space = getArea().getSpace();
	const FluidGroup* fluidGroup = space.fluid_getGroup(point, fluidType);
	if(fluidGroup->m_stable)
		return fluidGroup->m_drainQueue.m_set.highestZ() - floatDepth >= point.z();
	else
		return space.fluid_shapeIsMostlySurroundedByFluidOfTypeAtDistanceAboveLocationWithFacing(getShape(index), fluidType, floatDepth, point,  facing);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Speed Portables<Derived, Index, ReferenceIndex, isActors>::lead_getSpeed(const Index& index)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	Area& area = getArea();
	Actors& actors = area.getActors();
	ActorOrItemIndex wrapped = getActorOrItemIndex(index);
	if constexpr(isActors)
	{
 		const ActorOrItemIndex& isPiloting = actors.getIsPiloting(static_cast<const ActorIndex&>(index));
		if(isPiloting.exists())
			wrapped = isPiloting;
	}
	std::vector<ActorOrItemIndex> actorsAndItems;
	while(wrapped.exists())
	{
		actorsAndItems.push_back(wrapped);
		if(!wrapped.isLeading(area))
			break;
		wrapped = wrapped.getFollower(area);
	}
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), Mass::create(0), Mass::create(0));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
ActorIndex Portables<Derived, Index, ReferenceIndex, isActors>::getLineLeader(const Index& index)
{
	// Recursively traverse to the front of the line and return the leader.
	Items& items = getArea().getItems();
	ActorOrItemIndex leader = m_leader[index];
	assert(getActorOrItemIndex(index) != leader);
	if(leader.empty())
	{
		assert(m_follower[index].exists());
		if constexpr(!isActors)
		{
			// If the leader is an item then return it's pilot.
			const ActorIndex& pilot = items.pilot_get(leader.getItem());
			assert(pilot.exists());
			return pilot;
		}
		return ActorIndex::create(index.get());
	}
	if(leader.isFollowing(getArea()))
		assert(leader.getLeader(getArea()) != getActorOrItemIndex(index));
	assert(leader.isLeading(getArea()));
	assert(leader.getFollower(getArea()) == getActorOrItemIndex(index));
	if(leader.isActor())
		return getActors().getLineLeader(leader.getActor());
	else
		return getItems().getLineLeader(leader.getItem());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::setCarrier(const Index& index, const ActorOrItemIndex& carrier)
{
	assert(!m_carrier[index].exists());
	m_carrier[index] = carrier;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::maybeSetCarrier(const Index& index, const ActorOrItemIndex& carrier)
{
	if(m_carrier[index] == carrier)
		return;
	setCarrier(index, carrier);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::unsetCarrier(const Index& index, [[maybe_unused]] const ActorOrItemIndex& carrier)
{
	assert(m_carrier[index] == carrier);
	m_carrier[index].clear();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::maybeFall(const Index& index)
{
	Space& space = getArea().getSpace();
	const Point3D& location = getLocation(index);
	if(location.z() == 0)
		return;
	const Point3D below = location.below();
	const ShapeId& shape = getShape(index);
	const CuboidSet& occupied = this->m_occupied[index];
	const Facing4& facing = getFacing(index);
	if(space.shape_canFitEverOrCurrentlyDynamic(below, shape, facing, occupied) && !canFloatAt(index, location, facing))
		fall(index);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::fall(const Index& index)
{
	Space& space = getArea().getSpace();
	const ShapeId& shape = getShape(index);
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
Mass Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_getMass(const Index& index) const
{
	const Area& area = getArea();
	Mass output = Mass::create(0);
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		output += onDeck.getMass(area);
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onSetLocation(const Index& index, const Point3D& previousLocation, const Facing4& previousFacing)
{
	Area& area = getArea();
	Space& space = area.getSpace();
	const Point3D& newLocation = getLocation(index);
	assert(newLocation.exists());
	const Facing4& newFacing = getFacing(index);
	if(space.isExposedToSky(newLocation))
		this->m_onSurface.set(index);
	else
		this->m_onSurface.maybeUnset(index);
	// Move decks attached to this portable.
	const DeckId& deckId = m_hasDecks[index];
	if(deckId.exists())
	{
		assert(previousLocation.exists());
		Offset3D offset = previousLocation.offsetTo(getLocation(index));
		area.m_decks.shift(area, deckId, offset, Distance::create(1), previousLocation, previousFacing, newFacing);
	}
	else
	{
		// TODO: move this to onInsertIntoPoints method.
		const OffsetCuboidSet& deckOffsets = static_cast<Derived*>(this)->getDeckOffsets(index);
		if(!deckOffsets.empty())
		{
			// Create decks for newly added.
			CuboidSet decks = CuboidSet::create(space.offsetBoundry(), newLocation, newFacing, deckOffsets);
			m_hasDecks[index] = area.m_decks.registerDecks(area, decks, getActorOrItemIndex(index));
		}
	}
	// Update which deck this portable is on.
	const DeckId& onDeckOf = area.m_decks.queryDeckId(getLocation(index));
	DeckId onDeckOfPrevious = previousLocation.exists() ? area.m_decks.queryDeckId(previousLocation) : DeckId::null();
	if(onDeckOfPrevious == deckId)
		onDeckOfPrevious.clear();
	if(onDeckOf != onDeckOfPrevious)
	{
		if(onDeckOf == DeckId::null())
			onDeck_clear(index);
		else
		{
			const ActorOrItemIndex& onDeckOfIndex = area.m_decks.getForId(onDeckOf);
			onDeck_set(index, onDeckOfIndex);
		}
	}
	// Set floating.
	// TODO: opitmize this.
	if constexpr(!isActors)
	{
		const FluidTypeId& fluidType = getFluidTypeCanFloatInAt(index, newLocation, newFacing);
		if(fluidType.exists())
		{
			if(!isFloating(index))
				setFloating(index, fluidType);
		}
		else if(isFloating(index))
			setNotFloating(index);
	}
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::updateIndexInCarrier(const Index& oldIndex, const Index& newIndex)
{
	if(m_carrier[newIndex].isActor())
	{
		// Carrier is actor, either via canPickUp or equipmentSet.
		const ActorIndex& actor = ActorIndex::cast(m_carrier[newIndex].get());
		Actors& actors = getActors();
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
		const ItemIndex& item = ItemIndex::cast(m_carrier[newIndex].get());
		Items& items = getItems();
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
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_reserve(const Index& index, CanReserve& canReserve, const Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->reserveFor(canReserve, quantity, std::move(callback));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_unreserve(const Index& index, CanReserve& canReserve, const Quantity quantity)
{
	m_reservables[index]->clearReservationFor(canReserve, quantity);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_unreserveFaction(const Index& index, const FactionId& faction)
{
	m_reservables[index]->clearReservationsFor(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_maybeUnreserve(const Index& index, CanReserve& canReserve, const Quantity quantity)
{
	m_reservables[index]->maybeClearReservationFor(canReserve, quantity);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_unreserveAll(const Index& index)
{
	m_reservables[index]->clearAll();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_setDishonorCallback(const Index& index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	m_reservables[index]->setDishonorCallbackFor(canReserve, std::move(callback));
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::reservable_merge(const Index& index, Reservable& other)
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
		{"isOnDeckOf", m_isOnDeckOf}
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
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_hasAnyReservations(const Index& index) const
{
	return m_reservables[index]->hasAnyReservations();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_exists(const Index& index, const FactionId& faction) const
{
	return m_reservables[index]->hasAnyReservationsWith(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_existsFor(const Index& index, const CanReserve& canReserve) const
{
	return m_reservables[index]->hasAnyReservationsFor(canReserve);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
bool Portables<Derived, Index, ReferenceIndex, isActors>::reservable_isFullyReserved(const Index& index, const FactionId& faction) const
{
	return m_reservables[index]->isFullyReserved(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
Quantity Portables<Derived, Index, ReferenceIndex, isActors>::reservable_getUnreservedCount(const Index& index, const FactionId& faction) const
{
	return m_reservables[index]->getUnreservedCount(faction);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_subscribe(const Index& index, HasOnDestroySubscriptions& hasSubscriptions)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>(getReference(index));
	hasSubscriptions.subscribe(*m_destroy[index].get());
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_subscribeThreadSafe(const Index& index, HasOnDestroySubscriptions& hasSubscriptions)
{
	std::lock_guard<std::mutex> lock(HasOnDestroySubscriptions::m_mutex);
	onDestroy_subscribe(index, hasSubscriptions);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_unsubscribe(const Index& index, HasOnDestroySubscriptions& hasSubscriptions)
{
	m_destroy[index]->unsubscribe(hasSubscriptions);
	if(m_destroy[index]->empty())
		m_destroy[index] = nullptr;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_unsubscribeAll(const Index& index)
{
	m_destroy[index]->unsubscribeAll();
	m_destroy[index] = nullptr;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>
void Portables<Derived, Index, ReferenceIndex, isActors>::onDestroy_merge(const Index& index, OnDestroy& other)
{
	if(m_destroy[index] == nullptr)
		m_destroy[index] = std::make_unique<OnDestroy>(getReference(index));
	m_destroy[index]->merge(other);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>DeckId Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_createDecks(const Index& index, const CuboidSet& cuboidSet)
{
	Area& area = getArea();
	DeckId output = area.m_decks.registerDecks(area, cuboidSet, ActorOrItemIndex::create(index));
	m_hasDecks[index] = output;
	return output;
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_destroyDecks(const Index& index)
{
	assert(m_hasDecks[index].exists());
	Area& area = getArea();
	area.m_decks.unregisterDecks(area, m_hasDecks[index]);
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_clear(const Index& index)
{
	const ActorOrItemIndex& onDeckOf = m_isOnDeckOf[index];
	assert(onDeckOf.isItem());
	const ItemIndex& onDeckOfItem = onDeckOf.getItem();
	Items& items = getItems();
	items.onDeck_removeFromOnDeck(onDeckOfItem, getActorOrItemIndex(index));
	const ActorIndex& pilot = items.pilot_get(onDeckOfItem);
	if(pilot.exists())
		getActors().move_setMoveSpeedActual(pilot, items.vehicle_getSpeed(onDeckOfItem));
	m_isOnDeckOf[index].clear();
}
template<class Derived, class Index, class ReferenceIndex, bool isActors>void Portables<Derived, Index, ReferenceIndex, isActors>::onDeck_set(const Index& index, const ActorOrItemIndex& onDeckOf)
{
	if(m_isOnDeckOf[index].exists())
		onDeck_clear(index);
	m_isOnDeckOf[index] = onDeckOf;
	assert(onDeckOf.isItem());
	const ItemIndex& onDeckOfItem = onDeckOf.getItem();
	Items& items = getItems();
	items.onDeck_insertIntoOnDeck(onDeckOfItem, getActorOrItemIndex(index));
	const ActorIndex& pilot = items.pilot_get(onDeckOfItem);
	if(pilot.exists())
		getActors().move_setMoveSpeedActual(pilot, items.vehicle_getSpeed(onDeckOfItem));
}