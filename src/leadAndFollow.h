#pragma once

#include <queue>

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
	bool isFollowing() const { return m_canLead != nullptr; }
};
class CanLead
{
	HasShape& m_hasShape;
	CanFollow* m_canFollow;
	std::queue<Block*> m_locationQueue;
public:
	CanLead(HasShape& a) : m_hasShape(a), m_canFollow(nullptr) { }
	// Call from Block::enter.
	void onMove();
	// Use in canEnterCurrently to prevent leader from moving too far ahead.
	bool isFollowerKeepingUp() const;
	bool isLeading() const;
	HasShape& getFollower() const;
	uint32_t getMoveSpeed() const;
};
