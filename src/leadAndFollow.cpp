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
bool CanLead::isLeading(HasShape& hasShape) const  { return m_canFollow != nullptr && &hasShape.m_canFollow == m_canFollow; }
HasShape& CanLead::getFollower() const { return m_canFollow->m_hasShape; }
// Class method.
uint32_t CanLead::getMoveSpeedForGroupWithAddedMass(std::vector<const HasShape*>& actorsAndItems, uint32_t addedRollingMass, uint32_t addedDeadMass)
{
	uint32_t rollingMass = addedRollingMass;
	uint32_t deadMass = addedDeadMass;
	uint32_t carryMass = 0;
	uint32_t lowestMoveSpeed = 0;
	for(const HasShape* hasShape : actorsAndItems)
	{
		if(hasShape->isItem())
		{
			Mass mass = static_cast<const Item&>(*hasShape).getMass();
			static const MoveType& rolling = MoveType::byName("rolling");
			if(hasShape->getMoveType() == rolling)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(hasShape->isActor());
			const Actor& actor = static_cast<const Actor&>(*hasShape);
			if(actor.m_canMove.canMove())
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
uint32_t CanLead::getMoveSpeed() const
{
	assert(!m_hasShape.m_canFollow.isFollowing());
	assert(m_hasShape.m_canLead.isLeading());
	HasShape* hasShape = &m_hasShape;
	std::vector<const HasShape*> actorsAndItems;
	while(true)
	{
		actorsAndItems.push_back(hasShape);
		if(!hasShape->m_canLead.isLeading())
			break;
		hasShape = &hasShape->m_canLead.getFollower();
	}
	return getMoveSpeedForGroupWithAddedMass(actorsAndItems, 0);
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
void CanFollow::unfollowIfFollowing()
{
	if(m_canLead != nullptr)
		unfollow();
}
