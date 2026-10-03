/*
 * A non virtual shared base class for Actor and Item.
 * Handles Reservable, but not CanReserve, OnDestroy, lead and follow and MoveType.
 */
#pragma once
#include "dataStructures/strongVector.h"
#include "eventSchedule.hpp"
#include "numericTypes/index.h"
#include "threadedTask.hpp"
#include "hasShapes.h"
#include "numericTypes/types.h"
#include "reservable.h"
#include "onDestroy.h"
#include "actorOrItemIndex.h"

struct MoveType;
class MoveEvent;
class Area;
class PathThreadedTask;
class HasOnDestroySubscriptions;
class Actors;
class Items;
class Space;
struct CuboidSet;
class FluidType;
class DeckRotationData;

template<class Derived, class Index, class ReferenceIndex, bool isActors>
class Portables : public HasShapes<Derived, Index>
{
protected:
	// Reservations for this thing.
	StrongVector<std::unique_ptr<Reservable>, Index> m_reservables;
	// Destruction callbacks for this thing.
	StrongVector<std::unique_ptr<OnDestroy>, Index> m_destroy;
	// The thing currently following this thing.
	StrongVector<ActorOrItemIndex, Index> m_follower;
	// The thing this thing is following.
	StrongVector<ActorOrItemIndex, Index> m_leader;
	// The thing this thing is being carried by or is cargo within.
	StrongVector<ActorOrItemIndex, Index> m_carrier;
	// The move type of this thing.
	StrongVector<MoveTypeId, Index> m_moveType;
	// Things which are on top of this thing and will be carried along when it moves.
	StrongVector<SmallSet<ActorOrItemIndex>, Index> m_onDeck;
	// The thing this thing is on top of and which it will be carried along by.
	StrongVector<ActorOrItemIndex, Index> m_isOnDeckOf;
	StrongVector<DeckId, Index> m_hasDecks;
	StrongVector<SmallSet<Project*>, Index> m_projectsOnDeck;
	StrongVector<CuboidSet, Index> m_cuboidsContainingFluidOnDeck;
	StrongVector<FluidTypeId, Index> m_floating;
	Portables(Area& area);
	void create(Index index, MoveTypeId moveType, const ShapeId shape, FactionId faction, bool isStatic, Quantity quantity);
	void log(Index index) const;
	void updateLeaderSpeedActual(Index index);
	void updateIndexInCarrier(Index oldIndex, Index newIndex);
	void updateStoredIndicesPortables(Index oldIndex, Index newIndex);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorOrItemIndex getActorOrItemIndex(Index index) const;
public:
	ReferenceData<Index, ReferenceIndex> m_referenceData;
	void forEachDataPortables(auto&& action)
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
		action(m_cuboidsContainingFluidOnDeck);
		action(m_floating);
	}
	void load(const Json& data);
	void followActor(Index index, ActorIndex actor);
	void followActorAllowTeleport(Index index, ActorIndex actor);
	void followItem(Index index, ItemIndex item);
	void followPolymorphic(Index index, ActorOrItemIndex ActorOrItem);
	void unfollow(Index index);
	void unfollowIfAny(Index index);
	void maybeLeadAndFollowDisband(Index index);
	void unfollowActor(Index index, ActorIndex actor);
	void unfollowItem(Index index, ItemIndex item);
	void leadAndFollowDisband(Index index);
	void setCarrier(Index index, ActorOrItemIndex carrier);
	void maybeSetCarrier(Index index, ActorOrItemIndex carrier);
	void unsetCarrier(Index index, ActorOrItemIndex carrier);
	void updateCarrierIndex(Index index, HasShapeIndex newIndex) { m_carrier[index].updateIndex(newIndex); }
	void setFollower(Index index, ActorOrItemIndex follower) { assert(!follower.isStatic(this->m_area)); m_follower[index] = follower; }
	void setLeader(Index index, ActorOrItemIndex leader) { assert(!leader.isStatic(this->m_area)); m_leader[index] = leader; }
	void unsetFollower(Index index, [[maybe_unused]] ActorOrItemIndex follower) { assert(m_follower[index] == follower); m_follower[index].clear(); }
	void unsetLeader(Index index, ActorOrItemIndex leader) { assert(m_follower[index] == leader); m_leader[index].clear(); }
	void fall(Index index);
	void maybeFall(Index index);
	void onSetLocation(Index index, Point3D previousLocation, Facing4 previousFacing);
	void onRemove(Index index);
	void setFloating(Index index, FluidTypeId fluidType, Distance depth);
	void unsetFloating(Index index);
	void setMoveType(Index index, MoveTypeId moveType);
	Index moveTo(Portables<Derived, Index, ReferenceIndex, isActors>& other, Index index);
	[[nodiscard]] ActorIndex getLineLeader(Index index) const;
	[[nodiscard]] MoveTypeId getMoveType(Index index) const { return m_moveType[index]; }
	[[nodiscard]] bool movingIndependently(Index index) const;
	[[nodiscard]] bool hasCarrier(Index index) const;
	[[nodiscard]] bool isFollowing(Index index) const;
	[[nodiscard]] bool isLeading(Index index) const;
	[[nodiscard]] bool isLeadingActor(Index index, ActorIndex actor) const;
	[[nodiscard]] bool isLeadingItem(Index index, ItemIndex item) const;
	[[nodiscard]] bool isLeadingPolymorphic(Index index, ActorOrItemIndex ActorOrItem) const;
	[[nodiscard]] ActorOrItemIndex getFollower(Index index) const { return m_follower[index]; }
	[[nodiscard]] ActorOrItemIndex getLeader(Index index) const { return m_leader[index]; }
	[[nodiscard]] Point3D getLocation(Index index) const { return HasShapes<Derived, Index>::getLocation(index); }
	[[nodiscard]] ShapeId getShape(Index index) const { return HasShapes<Derived, Index>::getShape(index); }
	[[nodiscard]] Facing4 getFacing(Index index) const { return HasShapes<Derived, Index>::getFacing(index); }
	[[nodiscard]] auto getReference(Index index) const -> Reference<Index, ReferenceIndex> { return m_referenceData.getReference(index); }
	[[nodiscard]] CuboidSet getOccupiedCombined(Index index) const;
	[[nodiscard]] MapWithCuboidKeys<CollisionVolume> getOccupiedCombinedWithVolumes(Index index) const;
	// Floating.
	[[nodiscard]] Distance floatsInAtDepth(Index index, FluidTypeId fluidType) const;
	[[nodiscard]] bool canFloatAt(Index index, Point3D point, Facing4 facing) const;
	[[nodiscard]] FluidTypeId getFluidTypeCanFloatInAt(Index index, Point3D point, Facing4 facing) const;
	[[nodiscard]] bool canFloatAtInFluidTypeWithFacing(Index index, Point3D point, FluidTypeId fluidType, Facing4 facing) const;
	[[nodiscard]] bool isFloating(Index index) const { return m_floating[index].exists(); }
	void setNotFloating(Index index) { m_floating[index].clear(); }
	// For testing.
	[[nodiscard]] Speed lead_getSpeed(Index index) const;
	// Reservations.
	// Quantity defaults to 0, which becomes maxReservations;
	void reservable_reserve(Index index, CanReserve& canReserve, Quantity quantity = Quantity::create(1), std::unique_ptr<DishonorCallback> callback = nullptr);
	void reservable_unreserve(Index index, CanReserve& canReserve, Quantity quantity = Quantity::create(1));
	void reservable_unreserveFaction(Index index, FactionId faction);
	void reservable_maybeUnreserve(Index index, CanReserve& canReserve, Quantity quantity = Quantity::create(1));
	void reservable_unreserveAll(Index index);
	void reservable_setDishonorCallback(Index index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	void reservable_merge(Index index, Reservable& other);
	[[nodiscard]] bool reservable_hasAnyReservations(Index index) const;
	[[nodiscard]] bool reservable_exists(Index index, FactionId faction) const;
	[[nodiscard]] bool reservable_existsFor(Index index, const CanReserve& canReserve) const;
	[[nodiscard]] bool reservable_isFullyReserved(Index index, FactionId faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(Index index, FactionId faction) const;
	// On Destroy.
	void onDestroy_subscribe(Index index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_subscribeThreadSafe(Index index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribe(Index index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribeAll(Index index);
	void onDestroy_merge(Index index, OnDestroy& other);
	// On deck.
	void onDeck_updateIsOnDeckOf(Index index, ActorOrItemIndex value) { m_isOnDeckOf[index] = value; }
	void onDeck_updateIndex(Index index, ActorOrItemIndex oldValue, ActorOrItemIndex newValue) { m_onDeck[index].update(oldValue, newValue); }
	// These two are for internal use only.
	void onDeck_removeFromOnDeck(Index index, ActorOrItemIndex value) { m_onDeck[index].erase(value); }
	void onDeck_insertIntoOnDeck(Index index, ActorOrItemIndex value) { m_onDeck[index].insert(value); }
	void onDeck_clear(Index index);
	void onDeck_set(Index index, ActorOrItemIndex onDeckOf);
	void onDeck_destroyDecks(Index index);
	void onDeck_recordPointsContainingFluid(Index index, const CuboidSet& points) { m_cuboidsContainingFluidOnDeck[index].addAll(points); }
	void onDeck_erasePointsContainingFluid(Index index, const CuboidSet& points) { m_cuboidsContainingFluidOnDeck[index].maybeRemoveAll(points); }
	[[nodiscard]] DeckId onDeck_createDecks(Index index, const CuboidSet& cuboidSet);
	[[nodiscard]] ActorOrItemIndex onDeck_getIsOnDeckOf(Index index) const { return m_isOnDeckOf[index]; }
	[[nodiscard]] bool onDeck_isOnDeck(Index index) const { return m_isOnDeckOf[index].exists(); }
	[[nodiscard]] bool onDeck_hasDecks(Index index) const { return m_hasDecks[index].exists(); }
	[[nodiscard]] const SmallSet<ActorOrItemIndex>& onDeck_get(Index index) const { return m_onDeck[index]; }
	[[nodiscard]] bool onDeck_hasAnyContent(Index index) const { return !m_onDeck[index].empty(); }
	[[nodiscard]] Mass onDeck_getMass(Index index) const;
	[[nodiscard]] const SmallSet<Project*>& onDeck_getProjects(Index index) const { return m_projectsOnDeck[index]; }
	[[nodiscard]] const CuboidSet& onDeck_getCuboidsContainingFluid(Index index) const { return m_cuboidsContainingFluidOnDeck[index]; }
};
class PortablesHelpers
{
public:
	// Static methods.
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass, Mass addedFloatingMass, Mass addedDeadMass);
	[[nodiscard]] static Speed getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems);
};