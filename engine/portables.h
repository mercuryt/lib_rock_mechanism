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

class Portables : public HasShapes
{
protected:
	DataVector<std::unique_ptr<Reservable>, HasShapeIndex> m_reservables;
	DataVector<std::unique_ptr<OnDestroy>, HasShapeIndex> m_destroy;
	DataVector<ActorOrItemIndex, HasShapeIndex> m_follower;
	DataVector<ActorOrItemIndex, HasShapeIndex> m_leader;
	DataVector<ActorOrItemIndex, HasShapeIndex> m_carrier;
	DataVector<MoveTypeId, HasShapeIndex> m_moveType;
	bool m_isActors;
	Portables(Area& area, bool isActors);
	void create(const HasShapeIndex& index, const MoveTypeId& moveType, const ShapeId& shape, const FactionId& faction, bool isStatic, const Quantity& quantity);
	void log(const HasShapeIndex& index) const;
	void updateLeaderSpeedActual(const HasShapeIndex& index);
	void updateIndexInCarrier(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex);
	void resize(const HasShapeIndex& newSize);
	void moveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorOrItemIndex getActorOrItemIndex(const HasShapeIndex& index);
	[[nodiscard]] ActorIndex getLineLeader(const HasShapeIndex& index);
public:
	void load(const Json& data);
	void followActor(const HasShapeIndex& index, const ActorIndex& actor);
	void followItem(const HasShapeIndex& index, const ItemIndex& item);
	void followPolymorphic(const HasShapeIndex& index, const ActorOrItemIndex& ActorOrItem);
	void unfollow(const HasShapeIndex& index);
	void unfollowIfAny(const HasShapeIndex& index);
	void unfollowActor(const HasShapeIndex& index, const ActorIndex& actor);
	void unfollowItem(const HasShapeIndex& index, const ItemIndex& item);
	void maybeLeadAndFollowDisband(const HasShapeIndex& index);
	void leadAndFollowDisband(const HasShapeIndex& index);
	void setCarrier(const HasShapeIndex& index, ActorOrItemIndex carrier);
	void unsetCarrier(const HasShapeIndex& index, ActorOrItemIndex carrier);
	void updateCarrierIndex(const HasShapeIndex& index, HasShapeIndex newIndex) { m_carrier[index].updateIndex(newIndex); }
	[[nodiscard]] MoveTypeId getMoveType(const HasShapeIndex& index) const { return m_moveType[index]; }
	[[nodiscard]] bool isFollowing(const HasShapeIndex& index) const;
	[[nodiscard]] bool isLeading(const HasShapeIndex& index) const;
	[[nodiscard]] bool isLeadingActor(const HasShapeIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool isLeadingItem(const HasShapeIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool isLeadingPolymorphic(const HasShapeIndex& index, const ActorOrItemIndex& ActorOrItem) const;
	[[nodiscard]] bool lead_allCanMove(const HasShapeIndex& index) const;
	[[nodiscard]] ActorOrItemIndex getFollower(const HasShapeIndex& index) { return m_follower[index]; }
	[[nodiscard]] ActorOrItemIndex getLeader(const HasShapeIndex& index) { return m_leader[index]; }
	[[nodiscard]] const  ActorOrItemIndex getFollower(const HasShapeIndex& index) const { return m_follower[index]; }
	[[nodiscard]] const ActorOrItemIndex getLeader(const HasShapeIndex& index) const { return m_leader[index]; }
	// For testing.
	[[nodiscard]] Speed lead_getSpeed(const HasShapeIndex& index);
	// Reservations.
	// Quantity defaults to 0, which becomes maxReservations;
	void reservable_reserve(const HasShapeIndex& index, CanReserve& canReserve, const Quantity quantity = Quantity::create(1), std::unique_ptr<DishonorCallback> callback = nullptr);
	void reservable_unreserve(const HasShapeIndex& index, CanReserve& canReserve, const Quantity quantity = Quantity::create(1));
	void reservable_unreserveFaction(const HasShapeIndex& index, const FactionId& faction);
	void reservable_maybeUnreserve(const HasShapeIndex& index, CanReserve& canReserve, const Quantity quantity = Quantity::create(1));
	void reservable_unreserveAll(const HasShapeIndex& index);
	void reservable_setDishonorCallback(const HasShapeIndex& index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	void reservable_merge(const HasShapeIndex&, Reservable& other);
	[[nodiscard]] bool reservable_hasAnyReservations(const HasShapeIndex& index) const;
	[[nodiscard]] bool reservable_exists(const HasShapeIndex& index, const FactionId& faction) const;
	[[nodiscard]] bool reservable_existsFor(const HasShapeIndex& index, const CanReserve& canReserve) const;
	[[nodiscard]] bool reservable_isFullyReserved(const HasShapeIndex& index, const FactionId& faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(const HasShapeIndex& index, const FactionId& faction) const;
	// On Destroy.
	void onDestroy_subscribe(const HasShapeIndex& index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_subscribeThreadSafe(const HasShapeIndex& index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribe(const HasShapeIndex& index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribeAll(const HasShapeIndex& index);
	void onDestroy_merge(const HasShapeIndex& index, OnDestroy& other);
	// Static methods.
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, const Mass& addedRollingMass, const Mass& addedDeadMass);
	[[nodiscard]] static Speed getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems);
};
