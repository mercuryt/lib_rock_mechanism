#pragma once
#include "util.h"
template<class Block, class Actor, class Plant>
class GrazeRequest
{
	Actor& m_actor;
	Block* m_result;
	uint32_t m_maxRange;
	GrazeRequest(Actor& a,  uint32_t mr) : m_actor(a), m_maxRange(mr) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return m_actor.m_location->taxiDistance(*block) < m_maxRange && block>anyoneCanEnterEver() && block->canEnterEver(m_actor);
		}
		auto destinationCondition = [&](Block* block)
		{
			for(Plant* plant : block->m_plants)
				if(m_actor.canEat(plant))
					return true;
			return false;
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_actor.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
			m_actor.onNothingToEat();
		else
			m_actor.setDestination(m_result);
	}
}
