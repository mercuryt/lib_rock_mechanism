#pragma once
#include "util.h"
#include "fluidType.h"
template<class Block, class Actor>
class DrinkRequest
{
	Actor& m_actor;
	const FluidType* m_fluidType;
	Block* m_result;
	uint32_t m_maxRange;
	DrinkRequest(Actor& a, const FluidType* ft,  uint32_t mr) : m_actor(a), m_fluidType(ft), m_maxRange(mr) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return m_actor.m_location->taxiDistance(*block) < m_maxRange && block>anyoneCanEnterEver() && block->canEnterEver(m_actor);
		}
		auto destinationCondition = [&](Block* block)
		{
			return block.m_fluids.contains(m_fluidType);
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_actor.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
			m_actor.onNothingToDrink();
		else
			m_actor.setDestination(m_result);
	}
}
