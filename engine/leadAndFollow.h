#pragma once

#include "eventSchedule.hpp"
#include "config.h"

#include <queue>
#include <cstdint>

struct DeserializationMemo;

//TODO: Multiple followers.
class HasShape;
class CanLead;
class CanFollowEvent;
class CanFollow final
{
	HasScheduledEvent<CanFollowEvent> m_event;
	HasShape& m_hasShape;
	CanLead* m_canLead = nullptr;
public:
	CanFollow(HasShape& a, Simulation& s);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void follow(CanLead& canLead, bool doAdjacentCheck = true);
	void unfollow();
	void unfollowIfFollowing();
	void maybeDisband();
	void disband();
	void tryToMove();
	[[nodiscard]] HasShape& getLineLeader();
	[[nodiscard]] HasShape& getLeader();
	[[nodiscard]] bool isFollowing() const { return m_canLead != nullptr; }
	friend class CanLead;
	friend class CanFollowEvent;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasEvent() const { return m_event.exists(); }
	[[maybe_unused, nodiscard]] Step getEventStep() const { return m_event.getStep(); }
};
class CanLead final
{
	std::deque<BlockIndex> m_locationQueue;
	HasShape& m_hasShape;
	CanFollow* m_canFollow = nullptr;
public:
	CanLead(HasShape& a) : m_hasShape(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// Call from BlockHasShapes::enter.
	void onMove();
	// Use in canEnterCurrently to prevent leader from moving too far ahead.
	[[nodiscard]] bool isFollowerKeepingUp() const;
	[[nodiscard]] bool isLeading() const;
	[[nodiscard]] bool isLeading(HasShape& hasShape) const;
	HasShape& getFollower();
	[[nodiscard]] const HasShape& getFollower() const;
	[[nodiscard]] Speed getMoveSpeed() const;
	[[nodiscard]] std::deque<BlockIndex>& getLocationQueue();
	friend class CanFollow;
	[[nodiscard]] static Speed getMoveSpeedForGroupWithAddedMass(std::vector<const HasShape*>& actorsAndItems, Mass addedRollingMass = 0, Mass addedDeadMass = 0);
	[[nodiscard]] static Speed getMoveSpeedForGroup(std::vector<const HasShape*>& actorsAndItems) { return getMoveSpeedForGroupWithAddedMass(actorsAndItems); }
};
class CanFollowEvent final : public ScheduledEvent
{
	//TODO: Cache moveTo.
	HasShape& m_hasShape;
public:
	CanFollowEvent(HasShape& hasShape, const Step start = 0);
	void execute();
	void clearReferences();
};
