#pragma once

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
	void follow(CanLead& canLead, bool doAdjacentCheck = true);
	void unfollow();
	void unfollowIfFollowing();
	void disband();
	void tryToMove();
	HasShape& getLineLeader();
	bool isFollowing() const { return m_canLead != nullptr; }
	friend class CanLead;
	friend class CanFollowEvent;
};
class CanLead final
{
	HasShape& m_hasShape;
	CanFollow* m_canFollow;
	std::deque<Block*> m_locationQueue;
public:
	CanLead(HasShape& a) : m_hasShape(a), m_canFollow(nullptr) { }
	// Call from BlockHasShapes::enter.
	void onMove();
	// Use in canEnterCurrently to prevent leader from moving too far ahead.
	bool isFollowerKeepingUp() const;
	bool isLeading() const;
	bool isLeading(HasShape& hasShape) const;
	HasShape& getFollower();
	const HasShape& getFollower() const;
	uint32_t getMoveSpeed() const;
	std::deque<Block*>& getLocationQueue();
	friend class CanFollow;
	static uint32_t getMoveSpeedForGroupWithAddedMass(std::vector<const HasShape*>& actorsAndItems, uint32_t addedRollingMass = 0, uint32_t addedDeadMass = 0);
	static uint32_t getMoveSpeedForGroup(std::vector<const HasShape*>& actorsAndItems) { return getMoveSpeedForGroupWithAddedMass(actorsAndItems); }
};
class CanFollowEvent final : public ScheduledEventWithPercent
{
	//TODO: Cache moveTo.
	HasShape& m_hasShape;
public:
	CanFollowEvent(HasShape& hasShape);
	void execute();
	void clearReferences();
};
