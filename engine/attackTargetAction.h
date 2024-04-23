#pragma once
#include "queuedAction.h"

template<class Actor>
class AttackTargetAction : QueuedAction<Actor>
{
	Actor& m_target;
	AttackTargetAction(Actor& a, Actor& t) : QueuedAction(a), m_target(t) { }
	void execute()
	{
		if(m_actor.m_maxAttackRange >= m_actor.m_location.getTaxiDistance(m_target))
			attack(m_actor, m_target);
		else
			m_actor.m_location->m_area->registerGetInRangeRequest(m_actor, m_target);
	}
	bool isComplete() const { return !m_target.isAlive() || m_target.m_incapacitated; }
}
