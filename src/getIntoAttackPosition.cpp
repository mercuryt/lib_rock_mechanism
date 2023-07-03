#include "getIntoAttackPosition.h"
void GetIntoAttackPositionThreadedTask::readStep()
{
	auto destinatonCondition = [&](Block* block)
	{
		return block->taxiDistance(m_target.m_location) <= m_range && block.hasLineOfSightTo(m_target.m_location);
	}
	m_route = path::getForActorToPredicate(m_actor, destinatonCondition);
}
void GetIntoAttackPositionThreadedTask::writeStep()
{
	if(m_route == nullptr)
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
		m_actor.setPath(m_route);
}
