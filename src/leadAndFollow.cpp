#include "leadAndFollow.h"
#include "block.h"
void CanLead::onMove()
{
	if(m_canFollow == nullptr)
		return;
	m_locationQueue.push_back(m_hasShape.m_location);
	Block& block = *m_locationQueue.back();
	if(block.m_hasShapes.canEnterCurrentlyFrom(m_canFollow->m_hasShape, *m_canFollow->m_hasShape.m_location))
	{
		m_canFollow->m_hasShape.setLocation(block);
		m_locationQueue.pop_front();
	}

}
bool CanLead::isFollowerKeepingUp() const { return m_hasShape.isAdjacentTo(m_canFollow->m_hasShape); }
bool CanLead::isLeading() const { return m_canFollow != nullptr; }
HasShape& CanLead::getFollower() const { return m_canFollow->m_hasShape; }
uint32_t CanLead::getMoveSpeed() const
{
	assert(!m_hasShape.m_canFollow.isFollowing());
	assert(m_hasShape.m_canLead.isLeading());
	uint32_t rollingMass = 0;
	uint32_t deadMass = 0;
	uint32_t carryMass = 0;
	uint32_t lowestMoveSpeed = 0;
	HasShape* hasShape = &m_hasShape;
	while(true)
	{
		if(hasShape->isItem())
		{
			uint32_t mass = static_cast<Item&>(*hasShape).getMass();
			static const MoveType& rolling = MoveType::byName("rolling");
			if(hasShape->getMoveType() == rolling)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(hasShape->isActor());
			Actor& actor = static_cast<Actor&>(*hasShape);
			if(actor.canMove())
			{
				carryMass += actor.m_attributes.getUnencomberedCarryMass();
				lowestMoveSpeed = std::min(lowestMoveSpeed, actor.m_attributes.getMoveSpeed());
			}
			else
				deadMass += actor.getMass();
		}
		if(!hasShape->m_canLead.isLeading())
			break;
		hasShape = &hasShape->m_canLead.getFollower();
	}
	uint32_t totalMass = deadMass + (rollingMass * Config::rollingMassModifier);
	if(totalMass <= carryMass)
		return lowestMoveSpeed;
	float ratio = totalMass / carryMass;
	if(ratio > Config::massCarryMinimumMovementRatio)
		return 0;
	return lowestMoveSpeed / ratio;
}
void CanFollow::follow(CanLead& canLead)
{
	assert(m_canLead == nullptr);
	assert(m_canLead->m_canFollow == nullptr);
	assert(m_hasShape.isAdjacentTo(m_canLead->m_hasShape));
	m_canLead = &canLead;
	m_canLead->m_canFollow = this;
	if(m_hasShape.isItem())
		m_hasShape.setStatic(false);
}
void CanFollow::unfollow()
{
	assert(m_canLead != nullptr);
	m_canLead->m_canFollow = nullptr;
	m_canLead->m_locationQueue.clear();
	m_canLead = nullptr;
	if(m_hasShape.isItem())
		m_hasShape.setStatic(true);
}
