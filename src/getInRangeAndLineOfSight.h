#pragma once

#include "threadedTask.h"

template<class Actor, class Block>
class GetInToRangeAndLineOfSightThreadedTask : ThreadedTask
{
	Actor& m_actor;
	Actor& m_target;
	uint32_t m_range;
	Block* m_destination;
	GetInToRangeAndLineOfSightThreadedTask(Actor& a, Actor& t, uint32_t r) : m_actor(a), m_target(t), m_range(r) {}
	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return block->anyoneCanEnterEver() && block->canEnterEver(m_actor);
		}
		auto destinatonCondition = [&](Block* block)
		{
			return block->taxiDistance(m_target.m_location) <= m_actor.m_maxAttackRange && block.hasLineOfSightTo(m_target.m_location);
		}
		m_destination = util::findWithPathCondition(pathCondition, destinatonCondition, *m_actor.m_location);
	}
	void writeStep()
	{
		//TODO: extract pathing from RouteRequest to use in this readStep and avoid having to wait for next step.
		if(m_destination != nullptr)
			m_actor.setDestination(*m_destination);
	}
}
