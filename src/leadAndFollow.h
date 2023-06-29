#pragma once
#include "hasShape.h"
//TODO: Multiple followers.
class CanLead;
class CanFollow
{
	HasShape& m_hasShape;
	CanLead* m_canLead;
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
	CanLead(HasShape& a) : m_hasShape(a), m_follower(nullptr) { }
	// Call from Block::enter.
	void onMove()
	{
		if(m_canFollow == nullptr)
			return;
		m_locationQueue.push_front(m_hasShape.m_locaton);
		Block& block = *m_locationQueue.back();
		if(block.hasShapeCanEnterCurrently(m_canFollow->m_hasShape))
		{
			block.enter(m_canFollow->m_hasShape);
			m_locationQueue.pop_back();
		}

	}
	// Use in canEnterCurrently to prevent leader from moving too far ahead.
	bool isFollowerKeepingUp() const { return m_hasShape.isAdjacentTo(canFollow->m_hasShape); }
	bool isLeading() const { return m_canFollow != nullptr; }
	HasShape& getFollower() const { return m_canFollow.m_hasShape; }
	uint32_t getMoveSpeed() const
	{
		assert(!m_hasShape.m_canFollow.isFollowing());
		assert(m_hasShape.m_canLead.isLeading());
		uint32_t rollingMass = 0;
		uint32_t deadMass = 0;
		uint32_t carryMass = 0;
		uint32_t lowestMoveSpeed = 0;
		HasShape* hasShape = m_hasShape;
		while(true)
		{
			if(hasShape.isItem())
			{
				uint32_t mass = static_cast<Item&>(*hasShape).getMass();
				if(hasShape.m_moveType.rolling)
					rollingMass += mass;
				else
					deadMass += mass;
			}
			else
			{
				assert(hasShape.isActor());
				Actor& actor = static_cast<Actor&>(*hasShape);
				if(actor.canMove())
				{
					carryMass += actor.m_attributes.getUnencomberedCarryMass();
					lowestMoveSpeed = std::min(lowestMoveSpeed, actor.m_attributes.getMoveSpeed());
				}
				else
					deadMass += actor.getMass();
			}
			if(!hasShape.m_canLead.isLeading())
				break;
			hasShape = hasShape.m_canLead.getFollower();
		}
		uint32_t totalMass = deadMass + (rollingMass * Config::rollingMassModifier);
		if(totalMass <= carryMass)
			return lowestMoveSpeed;
		float ratio = totalMass / carryMass;
		if(ratio > Config::massCarryMinimumMovementRatio)
			return 0;
		return lowestMoveSpeed / ratio;
	}
};
inline void CanFollow::follow(CanLead& canLead)
{
	assert(m_canLead == nullptr);
	assert(m_canLead->m_canFollow == nullptr);
	assert(m_hasShape.isAdjacentTo(m_canLead->m_hasShape));
	m_canLead = &canLead;
	m_canLead.m_canFollow = this;
}
inline void CanFollow::unfollow()
{
	assert(m_canLead != nullptr);
	m_canLead->m_canFollow = nullptr;
	m_canLead->m_locationQueue.empty();
	m_canLead = nullptr;
}
