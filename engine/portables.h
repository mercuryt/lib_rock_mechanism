/*
 * A non virtual shared base class for Actor and Item.
 * Handles Reservable, but not CanReserve, OnDestroy, lead and follow and MoveType.
 */
#pragma once
#include "dataVector.h"
#include "eventSchedule.hpp"
#include "index.h"
#include "threadedTask.hpp"
#include "hasShapes.h"
#include "types.h"
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
class Blocks;

template<class Derived, class Index, class ReferenceIndex>
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
	StrongVector<ShapeId, Index> m_pathingShape;
	// Is this thing an actor?
	bool m_isActors;
	Portables(Area& area, bool isActors);
	void create(const Index& index, const MoveTypeId& moveType, const ShapeId& shape, const FactionId& faction, bool isStatic, const Quantity& quantity);
	void log(const Index& index) const;
	void updateLeaderSpeedActual(const Index& index);
	void updateIndexInCarrier(const Index& oldIndex, const Index& newIndex);
	void updateStoredIndicesPortables(const Index& oldIndex, const Index& newIndex);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorOrItemIndex getActorOrItemIndex(const Index& index);
public:
	ReferenceData<Index, ReferenceIndex> m_referenceData;
	template<typename Action>
	void forEachDataPortables(Action&& action);
	void load(const Json& data);
	void followActor(const Index& index, const ActorIndex& actor);
	void followItem(const Index& index, const ItemIndex& item);
	void followPolymorphic(const Index& index, const ActorOrItemIndex& ActorOrItem);
	void unfollow(const Index& index);
	void unfollowIfAny(const Index& index);
	void maybeLeadAndFollowDisband(const Index& index);
	void unfollowActor(const Index& index, const ActorIndex& actor);
	void unfollowItem(const Index& index, const ItemIndex& item);
	void leadAndFollowDisband(const Index& index);
	void setCarrier(const Index& index, const ActorOrItemIndex& carrier);
	void maybeSetCarrier(const Index& index, const ActorOrItemIndex& carrier);
	void unsetCarrier(const Index& index, const ActorOrItemIndex& carrier);
	void updateCarrierIndex(const Index& index, const HasShapeIndex& newIndex) { m_carrier[index].updateIndex(newIndex); }
	void setFollower(const Index& index, const ActorOrItemIndex& follower) { m_follower[index] = follower; }
	void setLeader(const Index& index, const ActorOrItemIndex& leader) { return m_leader[index] = leader; }
	void unsetFollower(const Index& index, [[maybe_unused]] const ActorOrItemIndex& follower) { assert(m_follower[index] == follower); m_follower[index].clear(); }
	void unsetLeader(const Index& index, const ActorOrItemIndex& leader) { assert(m_follower[index] == leader); m_leader[index].clear(); }
	void fall(const Index& index);
	void onSetLocation(const Index& index, const Facing4& previousFacing, const SmallMap<ActorOrItemIndex, Offset3D>& onDeckWithOffsets);
	[[nodiscard]] ActorIndex getLineLeader(const Index& index);
	[[nodiscard]] const MoveTypeId& getMoveType(const Index& index) const { return m_moveType[index]; }
	[[nodiscard]] bool isFollowing(const Index& index) const;
	[[nodiscard]] bool isLeading(const Index& index) const;
	[[nodiscard]] bool isLeadingActor(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] bool isLeadingItem(const Index& index, const ItemIndex& item) const;
	[[nodiscard]] bool isLeadingPolymorphic(const Index& index, const ActorOrItemIndex& ActorOrItem) const;
	[[nodiscard]] ActorOrItemIndex& getFollower(const Index& index) { return m_follower[index]; }
	[[nodiscard]] ActorOrItemIndex& getLeader(const Index& index) { return m_leader[index]; }
	[[nodiscard]] const ActorOrItemIndex& getFollower(const Index& index) const { return m_follower[index]; }
	[[nodiscard]] const ActorOrItemIndex& getLeader(const Index &index) const { return m_leader[index]; }
	[[nodiscard]] Items& getItems() { return getArea().getItems(); }
	[[nodiscard]] Actors& getActors() { return getArea().getActors(); }
	[[nodiscard]] Area& getArea() { return HasShapes<Derived, Index>::getArea(); }
	[[nodiscard]] const Area& getArea() const { return HasShapes<Derived, Index>::getArea(); }
	[[nodiscard]] BlockIndex getLocation(const Index& index) const { return HasShapes<Derived, Index>::getLocation(index); }
	[[nodiscard]] ShapeId getShape(const Index& index) const { return HasShapes<Derived, Index>::getShape(index); }
	[[nodiscard]] Facing4 getFacing(const Index& index) const { return HasShapes<Derived, Index>::getFacing(index); }
	[[nodiscard]] auto getReference(const Index& index) -> Reference<Index, ReferenceIndex> const { return m_referenceData.getReference(index); }
	// For testing.
	[[nodiscard]] Speed lead_getSpeed(const Index& index);
	// Reservations.
	// Quantity defaults to 0, which becomes maxReservations;
	void reservable_reserve(const Index& index, CanReserve& canReserve, const Quantity quantity = Quantity::create(1), std::unique_ptr<DishonorCallback> callback = nullptr);
	void reservable_unreserve(const Index& index, CanReserve& canReserve, const Quantity quantity = Quantity::create(1));
	void reservable_unreserveFaction(const Index& index, const FactionId& faction);
	void reservable_maybeUnreserve(const Index& index, CanReserve& canReserve, const Quantity quantity = Quantity::create(1));
	void reservable_unreserveAll(const Index& index);
	void reservable_setDishonorCallback(const Index& index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	void reservable_merge(const Index&, Reservable& other);
	[[nodiscard]] bool reservable_hasAnyReservations(const Index& index) const;
	[[nodiscard]] bool reservable_exists(const Index& index, const FactionId& faction) const;
	[[nodiscard]] bool reservable_existsFor(const Index& index, const CanReserve& canReserve) const;
	[[nodiscard]] bool reservable_isFullyReserved(const Index& index, const FactionId& faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(const Index& index, const FactionId& faction) const;
	// On Destroy.
	void onDestroy_subscribe(const Index& index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_subscribeThreadSafe(const Index& index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribe(const Index& index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribeAll(const Index& index);
	void onDestroy_merge(const Index& index, OnDestroy& other);
	// On deck.
	void onDeck_updateIsOnDeckOf(const Index& index, const ActorOrItemIndex& value) { m_isOnDeckOf[index] = value; }
	void onDeck_updateIndex(const Index& index, const ActorOrItemIndex& oldValue, const ActorOrItemIndex& newValue) { m_onDeck[index].update(oldValue, newValue); }
	[[nodiscard]] const ActorOrItemIndex& onDeck_getIsOnDeckOf(const Index& index) const { return m_isOnDeckOf[index]; }
	[[nodiscard]] const SmallSet<ActorOrItemIndex>& onDeck_get(const Index& index) const { return m_onDeck[index]; }
	[[nodiscard]] Mass onDeck_getMass(const Index& index) const ;
};
class PortablesHelpers
{
public:
	// Static methods.
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, const Mass& addedRollingMass, const Mass& addedDeadMass);
	[[nodiscard]] static Speed getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems);
};