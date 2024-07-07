/*
 * A non virtual shared base class for Actor and Item.
 */
#pragma once
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "hasShapes.h"
#include "types.h"
#include "reservable.h"
#include "onDestroy.h"
#include "leadAndFollow.h"

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
	std::vector<std::unique_ptr<CanLead>> m_lead;
	std::vector<std::unique_ptr<CanFollow>> m_follow;
	std::vector<const MoveType*> m_moveType;
	Portables(Area& area);
	Portables(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] virtual Json toJson() const;
	void create(HasShapeIndex index, const MoveType& moveType, const Shape& shape, BlockIndex location, Facing facing, bool isStatic);
	void destroy(HasShapeIndex index);
	void log(HasShapeIndex index) const;
	virtual void resize(HasShapeIndex newSize);
	virtual void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] virtual bool indexCanBeMoved(HasShapeIndex index) const;
public:
	[[nodiscard]] const MoveType& getMoveType(HasShapeIndex index) const { return *m_moveType.at(index); }
	void followActor(HasShapeIndex index, ActorIndex actor, bool checkAdjacent = true);
	void followItem(HasShapeIndex index, ItemIndex item, bool checkAdjacent = true);
	void followPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem, bool checkAdjacent = true);
	void unfollow(HasShapeIndex index);
	void unfollowIfAny(HasShapeIndex index);
	void unfollowActor(HasShapeIndex index, ActorIndex actor);
	void unfollowItem(HasShapeIndex index, ItemIndex actor);
	void leadAndFollowDisband(HasShapeIndex index);
	[[nodiscard]] bool isFollowing(HasShapeIndex index) const;
	[[nodiscard]] bool isLeading(HasShapeIndex index) const;
	[[nodiscard]] bool isLeadingActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isLeadingItem(HasShapeIndex index, ItemIndex item) const;
	[[nodiscard]] bool isLeadingPolymorphic(HasShapeIndex index, ActorOrItemIndex actorOrItem) const;
	// Used by leadAndFollow.cpp.
	[[nodiscard]] CanLead& getCanLead(HasShapeIndex index) { return *m_lead.at(index).get(); }
	[[nodiscard]] CanFollow& getCanFollow(HasShapeIndex index) { return *m_follow.at(index).get(); }
	// For testing.
	[[nodiscard]] Speed lead_getSpeed(HasShapeIndex index);
	// Quantity defaults to 0, which becomes maxReservations;
	void reservable_reserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = 0, std::unique_ptr<DishonorCallback> callback = nullptr);
	void reservable_unreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = 0);
	void reservable_unreserveFaction(HasShapeIndex index, const Faction& faction);
	void reservable_maybeUnreserve(HasShapeIndex index, CanReserve& canReserve, Quantity quantity = 0);
	void reservable_unreserveAll(HasShapeIndex index);
	void reservable_setDishonorCallback(HasShapeIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	void reservable_merge(HasShapeIndex, Reservable& other);
	[[nodiscard]] bool reservable_hasAnyReservations(HasShapeIndex index) const;
	[[nodiscard]] bool reservable_exists(HasShapeIndex index, const Faction& faction) const;
	[[nodiscard]] bool reservable_isFullyReserved(HasShapeIndex index, const Faction& faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(HasShapeIndex index, const Faction& faction) const;
	void onDestroy_subscribe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribe(HasShapeIndex index, HasOnDestroySubscriptions& onDestroy);
	void onDestroy_unsubscribeAll(HasShapeIndex index);
	void onDestroy_merge(HasShapeIndex index, OnDestroy& other);
};
