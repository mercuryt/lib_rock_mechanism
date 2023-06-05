#pragma once
#include "util.h"
#include "queuedAction.h"
#include "eventSchedule.h"

template<class Animal>
class ScavengeEvent : public ScheduledEvent
{
	Animal& m_animal;
	ScavengeEvent(Animal& a) : ScheduledEvent(s_step + Const::stepsToScavenge), m_animal(a) {}
	void execute()
	{
		Actor* actor = getActor();
		if(actor == nullptr)
			m_animal.m_location->m_area->registerScavengeRequest(m_animal);
		uint32_t meatMassConsumed = std::min(actor->getMeatMass(), m_animal->getMassFoodRequested());
		actor.removeMeatMass(meatMassConsumed);
		m_animal.eat(meatMassConsumed);
		if(m_animal.getMassFoodRequestd() == 0)
			m_animal.finishedCurrentTask();
		else
			m_animal.m_location->m_area->registerScavengeRequest(m_animal);
	}
	Actor* getActor() const
	{
		
		for(Actor* actor : m_animal.m_location->m_actors)
			if(m_animal.canEat(*actor))
				return actor;
		for(Block* block : m_animal.m_location.m_adjacentsVector)
			for(Actor* actor : block->m_actors)
				if(m_animal.canEat(*actor))
					return actor;
		return nullptr;
	}
	void cancel()
	{
		m_animal.m_location->m_area->registerScavengeRequest(m_animal);
		ScheduledEvent::cancel();
	}
};
template<class Animal>
class ScavengeQueuedAction : public QueuedAction
{
public:
	Animal& m_animal;
	ScavengeQueuedAction(Animal& a) : QueuedAction(Config::eatPriority), m_animal(a) {}
	void execute()
	{
		// If next to edible actor then schedule scavenge event.
		for(Block* block : m_animal.m_location->m_adjacentsVector)
			for(Actor* actor : block->m_actors)
				if(m_animal.canEat(*actor))
				{
					m_animal.m_location->m_area.m_eventSchedule.schedule(std::make_unique<ScavengeEvent>(m_animal));
					return;
				}
		// Otherwise register scavenge request.
		m_animal.m_location->m_area->registerScavengeRequest(m_animal);
	}
};
// Gets created in a data structure in Area for threading the task of finding a scavenge target.
template<class Block, class Animal, class FluidType>
class ScavengeRequest
{
	Animal& m_animal;
	const FluidType& m_fluidType;
	Block* m_result;
	uint32_t m_maxRange;
	ScavengeRequest(Animal& a, const FluidType& ft,  uint32_t mr) : m_animal(a), m_fluidType(ft), m_result(nullptr), m_maxRange(mr) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return m_animal.m_location->taxiDistance(*block) < m_maxRange && block->anyoneCanEnterEver() && block->canEnterEver(m_animal);
		}
		auto destinationCondition = [&](Block* block)
		{
			for(Actor* actor : block->m_actors)
				if(actor.m_alive && m_animal.canEat(actor))
					return true;
			return false;
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_animal.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
		{
			if(m_animal.m_animalType.canHunt)
				m_animal.m_location->m_area->registerHuntRequest(m_animal);
			else
				m_animal.onNothingToEat();
		}
		else
		{
			m_animal.setDestination(m_result);
			m_animal.m_actionQueue.push_back(std::make_unique<ScavengeQueuedAction>(m_animal));
		}
			
	}
};
template<class Animal>
class ScavengeObjective : public Objective
{
	Animal& m_animal;
	ScavengeObjective(Animal& a) : Objective(Config::eatPriority), m_animal(a) {}
	void execute()
	{
		m_animal.m_location->m_area->registerScavengeRequest(m_animal);
	}
};
