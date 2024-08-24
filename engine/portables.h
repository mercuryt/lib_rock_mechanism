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
	void create(HasShapeIndex index, MoveTypeId moveType, ShapeId shape, FactionId faction, bool isStatic, Quantity quantity);
	void log(HasShapeIndex index) const;
	void updateLeaderSpeedActual(HasShapeIndex index);
	void updateIndexInCarrier(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ActorOrItemIndex getActorOrItemIndex(HasShapeIndex index);
	[[nodiscard]] ActorIndex getLineLeader(HasShapeIndex index);
public:
	void load(const Json& data);
	void followActor(HasShapeIndex index, ActorIndex actor);
	void followItem(HasShapeIndex index, ItemIndex item);
	void followPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem);
	void unfollow(HasShapeIndex index);
	void unfollowIfAny(HasShapeIndex index);
	void unfollowActor(HasShapeIndex index, ActorIndex actor);
	void unfollowItem(HasShapeIndex index, ItemIndex actor);
	void leadAndFollowDisband(HasShapeIndex index);
	void setCarrier(HasShapeIndex index, ActorOrItemIndex carrier);
	void unsetCarrier(HasShapeIndex index, ActorOrItemIndex carrier);
	void updateCarrierIndex(HasShapeIndex index, HasShapeIndex newIndex) { m_carrier[index].updateIndex(newIndex); }
	[[nodiscard]] MoveTypeId getMoveType(HasShapeIndex index) const { return m_moveType[index]; }
	[[nodiscard]] bool isFollowing(HasShapeIndex index) const;
	[[nodiscard]] bool isLeading(HasShapeIndex index) const;
	[[nodiscard]] bool isLeadingActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isLeadingItem(HasShapeIndex index, ItemIndex item) const;
	[[nodiscard]] bool isLeadingPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem) const;
	[[nodiscard]] bool lead_allCanMove(HasShapeIndex index) const;
	[[nodiscard]] ActorOrItemIndex getFollower(HasShapeIndex index) { return m_leader[index]; }
	[[nodiscard]] ActorOrItemIndex getLeader(HasShapeIndex index) { return m_follower[index]; }
	[[nodiscard]] const  ActorOrItemIndex getFollower(HasShapeIndex index) const { return m_leader[index]; }
	[[nodiscard]] const ActorOrItemIndex getLeader(HasShapeIndex index) const { return m_follower[index]; }
	// For testing.
	[[nodiscard]] Speed lead_getSpeed(HasShapeIndex index);
	// Reservations.
	// Quantity defaults to 0, which becomes maxReservations;
	void reservable_reserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = Quantity::create(1), std::unique_ptr<DishonorCallback> callback = nullptr);
	void reservable_unreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = Quantity::create(1));
	void reservable_unreserveFaction(HasShapeIndex index, const FactionId faction);
	void reservable_maybeUnreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = Quantity::create(1));
	void reservable_unreserveAll(HasShapeIndex index);
	void reservable_setDishonorCallback(HasShapeIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	void reservable_merge(HasShapeIndex, Reservable& other);
	[[nodiscard]] bool reservable_hasAnyReservations(HasShapeIndex index) const;
	[[nodiscard]] bool reservable_exists(HasShapeIndex index, const FactionId faction) const;
	[[nodiscard]] bool reservable_isFullyReserved(HasShapeIndex index, const FactionId faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(HasShapeIndex index, const FactionId faction) const;
	// On Destroy.
	void onDestroy_subscribe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_subscribeThreadSafe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribeAll(HasShapeIndex index);
	void onDestroy_merge(HasShapeIndex index, OnDestroy& other);
	// Static methods.
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass, Mass addedDeadMass);
	[[nodiscard]] static Speed getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems);
};
