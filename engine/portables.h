/*
 * A non virtual shared base class for Actor and Item.
 * Handles Reservable, but not CanReserve, OnDestroy, lead and follow and MoveType.
 */
#pragma once
#include "eventSchedule.hpp"
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
class ActorOrItemIndex;

class Portables : public HasShapes
{
protected:
	std::vector<std::unique_ptr<Reservable>> m_reservables;
	std::vector<std::unique_ptr<OnDestroy>> m_destroy;
	std::vector<ActorOrItemIndex> m_follower;
	std::vector<ActorOrItemIndex> m_leader;
	std::vector<ActorOrItemIndex> m_carrier;
	std::vector<const MoveType*> m_moveType;
	bool isActors;
	Portables(Area& area);
	void create(HasShapeIndex index, const MoveType& moveType, const Shape& shape, BlockIndex location, Facing facing, bool isStatic);
	void destroy(HasShapeIndex index);
	void log(HasShapeIndex index) const;
	void updateLeaderSpeedActual(HasShapeIndex index);
	void updateIndexInCarrier(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	virtual void resize(HasShapeIndex newSize);
	virtual void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
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
	void updateCarrierIndex(HasShapeIndex index, HasShapeIndex newIndex) { m_carrier.at(index).updateIndex(newIndex); }
	[[nodiscard]] const MoveType& getMoveType(HasShapeIndex index) const { return *m_moveType.at(index); }
	[[nodiscard]] bool isFollowing(HasShapeIndex index) const;
	[[nodiscard]] bool isLeading(HasShapeIndex index) const;
	[[nodiscard]] bool isLeadingActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isLeadingItem(HasShapeIndex index, ItemIndex item) const;
	[[nodiscard]] bool isLeadingPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem) const;
	[[nodiscard]] ActorOrItemIndex getFollower(HasShapeIndex index) { return m_leader.at(index); }
	[[nodiscard]] ActorOrItemIndex getLeader(HasShapeIndex index) { return m_follower.at(index); }
	// For testing.
	[[nodiscard]] Speed lead_getSpeed(HasShapeIndex index);
	// Quantity defaults to 0, which becomes maxReservations;
	void reservable_reserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = 1, std::unique_ptr<DishonorCallback> callback = nullptr);
	void reservable_unreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = 1);
	void reservable_unreserveFaction(HasShapeIndex index, const FactionId faction);
	void reservable_maybeUnreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = 1);
	void reservable_unreserveAll(HasShapeIndex index);
	void reservable_setDishonorCallback(HasShapeIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	void reservable_merge(HasShapeIndex, Reservable& other);
	[[nodiscard]] bool reservable_hasAnyReservations(HasShapeIndex index) const;
	[[nodiscard]] bool reservable_exists(HasShapeIndex index, const FactionId faction) const;
	[[nodiscard]] bool reservable_isFullyReserved(HasShapeIndex index, const FactionId faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(HasShapeIndex index, const FactionId faction) const;
	void onDestroy_subscribe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribeAll(HasShapeIndex index);
	void onDestroy_merge(HasShapeIndex index, OnDestroy& other);
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass, Mass addedDeadMass);
	[[nodiscard]] static Speed getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems);
};
