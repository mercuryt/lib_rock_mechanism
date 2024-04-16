#pragma once

#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "config.h"

#include <queue>
#include <cstdint>

class Block;

//TODO: Multiple followers.
class HasShape;
class CanLead;
class CanFollowEvent;
class CanFollow final
{
	HasShape& m_hasShape;
	CanLead* m_canLead;
	HasScheduledEvent<CanFollowEvent> m_event;
public:
	CanFollow(HasShape& a);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void follow(CanLead& canLead, bool doAdjacentCheck = true);
	void unfollow();
	void unfollowIfFollowing();
	void disband();
	void tryToMove();
	HasShape& getLineLeader();
	HasShape& getLeader();
	[[nodiscard]] bool isFollowing() const { return m_canLead != nullptr; }
	friend class CanLead;
	friend class CanFollowEvent;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasEvent() const { return m_event.exists(); }
	[[maybe_unused, nodiscard]] Step getEventStep() const { return m_event.getStep(); }
};
class CanLead final
{
	HasShape& m_hasShape;
	CanFollow* m_canFollow;
	std::deque<Block*> m_locationQueue;
public:
	CanLead(HasShape& a) : m_hasShape(a), m_canFollow(nullptr) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// Call from BlockHasShapes::enter.
	void onMove();
	// Use in canEnterCurrently to prevent leader from moving too far ahead.
	bool isFollowerKeepingUp() const;
	bool isLeading() const;
	bool isLeading(HasShape& hasShape) const;
	HasShape& getFollower();
	const HasShape& getFollower() const;
	Speed getMoveSpeed() const;
	std::deque<Block*>& getLocationQueue();
	friend class CanFollow;
	static Speed getMoveSpeedForGroupWithAddedMass(std::vector<const HasShape*>& actorsAndItems, Mass addedRollingMass = 0, Mass addedDeadMass = 0);
	static Speed getMoveSpeedForGroup(std::vector<const HasShape*>& actorsAndItems) { return getMoveSpeedForGroupWithAddedMass(actorsAndItems); }
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
