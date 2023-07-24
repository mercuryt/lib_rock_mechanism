#pragma once

#include <queue>
#include <cstdint>

class Block;

//TODO: Multiple followers.
class HasShape;
class CanLead;
class CanFollow
{
	HasShape& m_hasShape;
	CanLead* m_canLead;
public:
	CanFollow(HasShape& a) : m_hasShape(a), m_canLead(nullptr) { }
	void follow(CanLead& canLead);
	void unfollow();
	void unfollowIfFollowing();
	bool isFollowing() const { return m_canLead != nullptr; }
	friend class CanLead;
};
class CanLead
{
	HasShape& m_hasShape;
	CanFollow* m_canFollow;
	std::deque<Block*> m_locationQueue;
public:
	CanLead(HasShape& a) : m_hasShape(a), m_canFollow(nullptr) { }
	// Call from Block::enter.
	void onMove();
	// Use in canEnterCurrently to prevent leader from moving too far ahead.
	bool isFollowerKeepingUp() const;
	bool isLeading() const;
	bool isLeading(HasShape& hasShape) const;
	HasShape& getFollower() const;
	uint32_t getMoveSpeed() const;
	friend class CanFollow;
};
