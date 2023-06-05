#pragma once
#include "util.h"
#include "queuedAction.h"
#include "eventSchedule.h"

template<class Animal>
class HuntUpdateDestinationEvent : public ScheduledEvent
{
	Animal& m_animal;
	Actor& m_prey;
	HuntEvent(Animal& a, Actor& p) : ScheduledEvent(s_step + Const::stepsToHunt), m_animal(a), m_prey(p) {}
	void execute()
	{
		if(!m_animal.m_destination == m_prey.m_location)
			m_animal.setDestination(m_prey.m_location);
		m_animal.makeHuntEvent(m_prey);
	}
	~HuntEvent(){ m_animal.m_actionEvent = nullptr; }
};
// Gets created in a data structure in Area for threading the task of finding a hunt target.
template<class Block, class Animal, class FluidType>
class HuntRequest
{
	Animal& m_animal;
	const FluidType& m_fluidType;
	Actor* m_result;
	uint32_t m_maxRange;
	HuntRequest(Animal& a, const FluidType& ft,  uint32_t mr) : m_animal(a), m_fluidType(ft), m_result(nullptr), m_maxRange(mr) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return m_animal.m_location->taxiDistance(*block) < m_maxRange && block->anyoneCanEnterEver() && block->canEnterEver(m_animal);
		}
		auto destinationCondition = [&](Block* block)
		{
			for(Actor* actor : block->m_animal)
				if(m_animal.canEat(actor))
					return true;
			return false;
		}
		//TODO: util::findActorWithPathCondition
		Block* blockResult = util::findWithPathCondition(pathCondition, destinationCondition, *m_animal.m_location);
		for(Actor* actor : blockResult->m_actors)
			if(m_animal.canEat(actor))
				m_result = actor;
	}
	void writeStep()
	{
		if(m_result == nullptr)
		{
			m_animal.onNothingToEat();
		}
		else
		{
			// Update destination periodically to track movement.
			m_animal.makeHuntUpdateDestinationEvent(m_result);
			m_animal.setDestination(*m_result->m_location);
			m_animal.m_actionQueue.push_back(std::make_unsigned<KillAction>(m_animal, m_result);
			m_animal.m_actionQueue.push_back(std::make_unique<ScavengeQueuedAction>(m_animal));
		}
	}
};
template<class Animal>
class HuntObjective : public Objective
{
	Animal& m_animal;
	HuntObjective(Animal& a) : Objective(Config::eatPriority), m_animal(a) {}
	void execute()
	{
		m_animal.m_location->m_area->registerHuntRequest(m_animal);
	}
};
