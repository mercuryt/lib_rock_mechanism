#pragma once
#include "portables.h"
#include "actorOrItemIndex.h"
#include "actors/actors.h"
#include "deserializationMemo.h"
#include "index.h"
#include "items/items.h"
#include "area/area.h"
#include "onDestroy.h"
#include "reservable.h"
#include "simulation/simulation.h"
#include "types.h"
#include "moveType.h"
#include "hasShapes.hpp"
#include "reservable.hpp"
#include <cstdint>
#include <memory>

template<class Derived, class Index, class ReferenceIndex>
Portables<Derived, Index, ReferenceIndex>::Portables(Area& area, bool isActors) : HasShapes<Derived, Index>(area), m_isActors(isActors) { }
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::updateStoredIndicesPortables(const Index& oldIndex, const Index& newIndex)
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
template<class Derived, class Index, class ReferenceIndex>
template<typename Action>
void Portables<Derived, Index, ReferenceIndex>::forEachDataPortables(Action&& action)
{
	this->forEachDataHasShapes(action);
	action(m_reservables);
	action(m_destroy);
	action(m_follower);
	action(m_leader);
	action(m_carrier);
	action(m_moveType);
	action(m_onDeck);
	action(m_isOnDeckOf);
	action(m_hasDecks);
	action(m_projectsOnDeck);
	action(m_blocksContainingFluidOnDeck);
	action(m_floating);
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
	assert(m_onDeck[index].empty());
	assert(m_isOnDeckOf[index].empty());
	assert(m_hasDecks[index].empty());
	assert(m_projectsOnDeck[index].empty());
	assert(m_floating[index].empty());
	// Corresponding remove in Actors::destroy and Items::destroy.
	m_referenceData.add(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onRemove(const Index& index)
{
	if(m_hasDecks[index].exists())
	{
		Area& area = getArea();
		area.m_decks.unregisterDecks(area, m_hasDecks[index]);
		m_hasDecks[index].clear();
		m_projectsOnDeck[index].clear();
	}
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::setFloating(const Index& index, const FluidTypeId& fluidType)
{
	// Only dead actors float, otherwise they swim.
	if(m_isActors)
		assert(!getActors().isAlive(getActorOrItemIndex(index).getActor()));
	m_floating[index] = fluidType;
	static const MoveTypeId& floatingMoveType = MoveType::byName("floating");
	setMoveType(index, floatingMoveType);
	if(!m_isActors)
	{
		// TODO: update pilot speed.
	}
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unsetFloating(const Index& index)
{
	m_floating[index].clear();
	Derived::resetMoveType(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::setMoveType(const Index& index, const MoveTypeId& moveType)
{
	assert(m_moveType[index] != moveType);
	m_moveType[index] = moveType;
	getArea().m_hasTerrainFacades.maybeRegisterMoveType(moveType);
	maybeFall(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::log(const Index& index) const
{
	std::cout << ", moveType: " << MoveType::getName(m_moveType[index]);
	if(m_follower[index].exists())
		std::cout << ", leading: " << m_follower[index].toString();
	if(m_leader[index].exists())
		std::cout << ", following: " << m_leader[index].toString();
	if(m_carrier[index].exists())
		std::cout << ", carrier: " << m_carrier[index].toString();
	const BlockIndex& location = getLocation(index);
	if(location.exists())
	{
		const Blocks& blocks = getArea().getBlocks();
		std::cout << ", location: " << blocks.getCoordinates(location).toString();
		for(const BlockIndex& occupied : this->m_blocks[index])
			if(occupied != location)
				std::cout << "-" << blocks.getCoordinates(occupied).toString();
	}
	//if(!m_onDeck[index].empty())
		//std::cout << ", on deck " << m_onDeck[index].toString();
	if(m_isOnDeckOf[index].exists())
		std::cout << ", is on deck of " << m_isOnDeckOf[index].toString();
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
	assert(!isFollowing(index));
	assert(!isLeading(index));
	assert(!actors.isLeading(actor));
	// If following a pilot, follow the mount or vehicle instead.
	const ActorOrItemIndex& leaderPiloting = actors.getIsPiloting(actor);
	if(leaderPiloting.exists())
	{
		followPolymorphic(index, leaderPiloting);
		return;
	}
	// if piloting something then set that thing to follow rather then the pilot.
	if(m_isActors)
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
	actors.lineLead_appendToPath(lineLeader, this->m_location[index], this->m_facing[index]);
	actors.move_updateActualSpeed(lineLeader);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::followItem(const Index& index, const ItemIndex& item)
{
	Actors& actors = getActors();
	Items& items = getItems();
	assert(!isFollowing(index));
	assert(!isLeading(index));
	assert(!items.isLeading(item));
	// Only follow items which already have leaders;
	assert(items.isFollowing(item));
	// if piloting something then set that thing to follow rather then the pilot.
	if(m_isActors)
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
	actors.lineLead_appendToPath(lineLeader, this->m_location[index], this->m_facing[index]);
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
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::unfollowItem(const Index& index, const ItemIndex& item)
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
OccupiedBlocksForHasShape Portables<Derived, Index, ReferenceIndex>::getBlocksCombined(const Index& index) const
{
	const OccupiedBlocksForHasShape& occupied = HasShapes<Derived, Index>::getBlocks(index);
	OccupiedBlocksForHasShape output;
	const Area& area = getArea();
	uint toReserve = occupied.size();
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		toReserve += Shape::getPositions(onDeck.getShape(area)).size();
	output.reserve(toReserve);
	for(const BlockIndex& block : occupied)
		output.insert(block);
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		for(const BlockIndex& block : onDeck.getBlocks(area))
			output.insert(block);
	return output;
}
template<class Derived, class Index, class ReferenceIndex>
DistanceInBlocks Portables<Derived, Index, ReferenceIndex>::floatsInAtDepth(const Index& index, const FluidTypeId& fluidType) const
{
	const Mass mass = static_cast<const Derived*>(this)->getMass(index);
	CollisionVolume displacement = CollisionVolume::create(0);
	DistanceInBlocks output = DistanceInBlocks::create(0);
	const Density& fluidDensity = FluidType::getDensity(fluidType);
	const ShapeId& shape = HasShapes<Derived, Index>::getShape(index);
	DistanceInBlocks shapeHeight = Shape::getZSize(shape);
	SmallSet<Offset3D> previousLevel;
	SmallSet<Offset3D> nextLevel;
	while(fluidDensity < mass / displacement.toVolume())
	{
		auto thisLevel = Shape::getPositionsByZLevel(shape, output);
		if(thisLevel.size() < previousLevel.size() || shapeHeight < output)
		{
			// Hull ends here.
			return DistanceInBlocks::null();
		}
		// Anything directly above a recorded block is assumed to be eiter part of or contained within the hull.
		displacement += Config::maxBlockVolume * nextLevel.size();
		// Use previous level to ensure we don't record the same block twice.
		previousLevel.swap(nextLevel);
		nextLevel.clear();
		for(const OffsetAndVolume& offsetAndVolume : thisLevel)
		{
			if(nextLevel.contains(offsetAndVolume.offset))
				continue;
			Offset3D above = offsetAndVolume.offset;
			++above.z();
			nextLevel.insert(above);
			displacement += offsetAndVolume.volume;
		}
		++output;
	}
	return output;
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::canFloatAt(const Index& index, const BlockIndex& block) const
{
	return getFluidTypeCanFloatInAt(index, block).exists();
}
template<class Derived, class Index, class ReferenceIndex>
FluidTypeId Portables<Derived, Index, ReferenceIndex>::getFluidTypeCanFloatInAt(const Index& index, const BlockIndex& block) const
{
	const Blocks& blocks = getArea().getBlocks();
	for(const FluidData& fluidData : blocks.fluid_getAll(block))
	{
		if(canFloatAtInFluidType(index, block, fluidData.type))
			return fluidData.type;
	}
	return FluidTypeId::null();
}
template<class Derived, class Index, class ReferenceIndex>
bool Portables<Derived, Index, ReferenceIndex>::canFloatAtInFluidType(const Index& index, const BlockIndex& block, const FluidTypeId& fluidType) const
{
	const DistanceInBlocks& floatDepth = floatsInAtDepth(index, fluidType);
	if(floatDepth.empty())
		// Cannot float in this fluid at any depth.
		return false;
	BlockIndex current = block;
	const Blocks& blocks = getArea().getBlocks();
	auto& occupied = HasShapes<Derived, Index>::getBlocks(index);
	for(uint i = 0; floatDepth > i; ++i)
	{
		if(!blocks.fluid_contains(current, fluidType) && !occupied.contains(current))
			return false;
		current = blocks.getBlockAbove(current);
	}
	return true;
}
template<class Derived, class Index, class ReferenceIndex>
Speed Portables<Derived, Index, ReferenceIndex>::lead_getSpeed(const Index& index)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	Area& area = getArea();
	Actors& actors = area.getActors();
	ActorOrItemIndex wrapped = getActorOrItemIndex(index);
	if(m_isActors)
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
template<class Derived, class Index, class ReferenceIndex>
ActorIndex Portables<Derived, Index, ReferenceIndex>::getLineLeader(const Index& index)
{
	// Recursively traverse to the front of the line and return the leader.
	Items& items = getArea().getItems();
	ActorOrItemIndex leader = m_leader[index];
	assert(getActorOrItemIndex(index) != leader);
	if(leader.empty())
	{
		assert(m_follower[index].exists());
		if(!m_isActors)
		{
			// If the leader is an item then return it's pilot.
			const ActorIndex& pilot = items.pilot_get(leader.getItem());
			assert(pilot.exists());
			return pilot;
		}
		return ActorIndex::cast(index);
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
void Portables<Derived, Index, ReferenceIndex>::unsetCarrier(const Index& index, [[maybe_unused]] const ActorOrItemIndex& carrier)
{
	assert(m_carrier[index] == carrier);
	m_carrier[index].clear();
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::maybeFall(const Index& index)
{
	Blocks& blocks = getArea().getBlocks();
	if(!blocks.shape_moveTypeCanEnter(getLocation(index), getMoveType(index)))
		fall(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::fall(const Index& index)
{
	Blocks& blocks = getArea().getBlocks();
	const ShapeId& shape = getShape(index);
	const Facing4& facing = getFacing(index);
	BlockIndex location = getLocation(index);
	assert(!canFloatAt(index, location));
	assert(!isFloating(index));
	assert(location.exists());
	assert(blocks.shape_anythingCanEnterEver(location));
	DistanceInBlocks distance = DistanceInBlocks::create(0);
	BlockIndex next;
	const auto& occupied = this->getBlocks(index);
	while(true)
	{
		next = blocks.getBlockBelow(location);
		if(blocks.shape_canFitEverOrCurrentlyDynamic(next, shape, facing, occupied) && !canFloatAt(index, location))
		{
			location = next;
			++distance;
		}
		else
			break;
	}
	assert(distance != 0);
	auto blocksBelowEndPosition = blocks.shape_getBelowBlocksWithFacing(location, shape, facing);
	MaterialTypeId materialType = blocks.solid_getHardest(blocksBelowEndPosition);
	static_cast<Derived*>(this)->location_set(index, location, facing);
	static_cast<Derived*>(this)->takeFallDamage(index, distance, materialType);
	//TODO: dig out / destruct below impact.
}
template<class Derived, class Index, class ReferenceIndex>
Mass Portables<Derived, Index, ReferenceIndex>::onDeck_getMass(const Index& index) const
{
	const Area& area = getArea();
	Mass output = Mass::create(0);
	for(const ActorOrItemIndex& onDeck : m_onDeck[index])
		output += onDeck.getMass(area);
	return output;
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onSetLocation(const Index& index, const BlockIndex& previousLocation, const Facing4& previousFacing, DeckRotationData& onDeckRotationData)
{
	Area& area = getArea();
	Blocks& blocks = area.getBlocks();
	const BlockIndex& newLocation = getLocation(index);
	assert(newLocation.exists());
	const Facing4& newFacing = getFacing(index);
	if(blocks.isExposedToSky(newLocation))
		this->m_onSurface.set(index);
	else
		this->m_onSurface.unset(index);
	// Translate Actors, Items, Fluids and Projects on deck into new positions. Also translate their reservations, paths, etc.
	onDeckRotationData.reinstanceAtRotatedPosition(area, previousLocation, newLocation, previousFacing, newFacing);
	// Move decks attached to this portable.
	const DeckId& deckId = m_hasDecks[index];
	if(deckId.exists())
	{
		assert(previousLocation.exists());
		Offset3D offset = blocks.relativeOffsetTo(previousLocation, getLocation(index));
		area.m_decks.shift(area, deckId, offset, DistanceInBlocks::create(1), previousLocation, previousFacing, newFacing);
	}
	else
	{
		// TODO: move this to onInsertIntoBlocks method.
		const OffsetCuboidSet& deckOffsets = static_cast<Derived*>(this)->getDeckOffsets(index);
		if(!deckOffsets.empty())
		{
			// Create decks for newly added.
			CuboidSet decks(blocks, newLocation, newFacing, deckOffsets);
			m_hasDecks[index] = area.m_decks.registerDecks(area, decks, getActorOrItemIndex(index));
		}
	}
	// Update which deck this portable is on.
	const DeckId& onDeckOf = area.m_decks.getForBlock(getLocation(index));
	const DeckId onDeckOfPrevious = previousLocation.exists() ? area.m_decks.getForBlock(previousLocation) : DeckId::null();
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
	if(MoveType::getSwim(getMoveType(index)).empty())
	{
		const FluidTypeId& fluidType = getFluidTypeCanFloatInAt(index, newLocation);
		if(fluidType.exists())
		{
			if(!isFloating(index))
				setFloating(index, fluidType);
		}
		else if(isFloating(index))
			setNotFloating(index);
	}
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
template<class Derived, class Index, class ReferenceIndex>
DeckId Portables<Derived, Index, ReferenceIndex>::onDeck_createDecks(const Index& index, const CuboidSet& cuboidSet)
{
	DeckId output =  getArea().m_decks.registerDecks(cuboidSet);
	m_hasDecks[index] = output;
	return output;
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDeck_destroyDecks(const Index& index)
{
	assert(m_hasDecks[index].exists());
	getArea().m_decks.unregisterDeck(index);
}
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDeck_clear(const Index& index)
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
template<class Derived, class Index, class ReferenceIndex>
void Portables<Derived, Index, ReferenceIndex>::onDeck_set(const Index& index, const ActorOrItemIndex& onDeckOf)
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