#pragma once

#include "eventSchedule.hpp"
#include "config.h"
#include "numericTypes/types.h"
#include "actorOrItemIndex.h"

#include <queue>
#include <cstdint>

struct DeserializationMemo;

//TODO: Multiple followers.
class CanLead;
class Area;
class CanFollow final
{
	ActorOrItemIndex m_actorOrItemIndex;
	CanLead* m_canLead = nullptr;
public:
	CanFollow(ActorOrItemIndex index);
	void load(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void follow(Area& area, CanLead& canLead, bool doAdjacentCheck = true);
	void unfollow(Area& area);
	void unfollowIfFollowing(Area& area);
	void maybeDisband(Area& area);
	void disband(Area& area);
	[[nodiscard]] ActorOrItemIndex getLineLeader(Area& area);
	[[nodiscard]] ActorOrItemIndex getLeader();
	[[nodiscard]] bool isFollowing() const { return m_canLead != nullptr; }
	friend class CanLead;
};
class CanLead final
{
	std::deque<BlockIndex> m_locationQueue;
	ActorOrItemIndex m_actorOrItemIndex;
	CanFollow* m_canFollow = nullptr;
public:
	CanLead(ActorOrItemIndex index) : m_actorOrItemIndex(index) { }
	void load(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// Use in Actors::move_callback to prevent the line leader from moving if a follower can't.
	[[nodiscard]] bool canMove(Area& area);
	[[nodiscard]] bool isLeading() const;
	[[nodiscard]] bool isLeadingActor(ActorIndex actor) const;
	[[nodiscard]] bool isLeadingItem(ItemIndex item) const;
	[[nodiscard]] bool isLeadingPolymorphic(ActorOrItemIndex hasShape) const;
	[[nodiscard]] ActorOrItemIndex getFollower();
	[[nodiscard]] const ActorOrItemIndex& getFollower() const;
	[[nodiscard]] Speed getMoveSpeed(Area& area) const;
	[[nodiscard]] std::deque<BlockIndex>& getLocationQueue(Area& area);
	[[nodiscard]] std::unordered_set<BlockIndex> getOccuiped(Area& area);
	friend class CanFollow;
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass = 0, Mass addedDeadMass = 0);
	[[nodiscard]] static Speed getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems) { return getMoveSpeedForGroupWithAddedMass(area, actorsAndItems); }
};
