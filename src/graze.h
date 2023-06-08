#pragma once
#include "util.h"
#include "queuedAction.h"
#include "eventSchedule.h"

template<class Animal, Plant>
class GrazeEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
	GrazeEvent(Animal& a) : ScheduledEvent(s_step + Const::stepsToGraze), m_animal(a) {}
	void execute()
	{
		Plant* plant = getPlant();
		if(plant == nullptr)
			m_animal.m_location->m_area->registerGrazeRequest(m_animal);
		uint32_t foliageMassAvalible = plant->getFoliageMass();
		uint32_t foliageMassConsumed = std::min(plant->getFoliageMass(), m_animal->getMassFoodRequested());
		plant.removeFoliageMass(foliageMassConsumed);
		m_animal.eat(foliageMassConsumed);
		if(m_animal.getMassFoodRequestd() == 0)
			m_animal.finishedCurrentTask();
		else
			m_animal.m_location->m_area->registerGrazeRequest(m_animal);
	}
	Plant* getPlant() const
	{
		
		for(Plant* plant : m_animal.m_location->m_plants)
			if(m_actor.canEat(*plant))
				return plant;
		for(Block* block : m_animal.m_location.m_adjacentsVector)
			for(Plant* plant : block->m_plants)
				if(m_actor.canEat(*plant))
					return plant;
		return nullptr;
	}
	void cancel()
	{
		m_animal.m_location->m_area->registerGrazeRequest(m_animal);
		ScheduledEvent::cancel();
	}
};
template<class Animal, class Plant>
class GrazeQueuedAction : public QueuedAction
{
public:
	Animal& m_animal;
	GrazeQueuedAction(Animal& a) : QueuedAction(Config::eatPriority), m_animal(a) {}
	void execute()
	{
		// If next to edible plant then schedule graze event.
		for(Block* block : m_animal.m_location->m_adjacentsVector)
			for(Plant* plant : block->m_plants)
				if(m_animal.canEat(*plant))
				{
					m_actor.m_location->m_area.m_eventSchedule.schedule(std::make_unique<GrazeEvent>(m_actor));
					return;
				}
		// Otherwise register graze request.
		m_animal.m_location->m_area->registerGrazeRequest(m_animal);
	}
};
// Gets created in a data structure in Area for threading the task of finding a graze target.
template<class Block, class Animal, class FluidType>
class GrazeRequest
{
	Animal& m_animal;
	const FluidType& m_fluidType;
	Block* m_result;
	uint32_t m_maxRange;
	GrazeRequest(Animal& a, const FluidType& ft,  uint32_t mr) : m_animal(a), m_fluidType(ft), m_result(nullptr), m_maxRange(mr) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return m_animal.m_location->taxiDistance(*block) < m_maxRange && block>anyoneCanEnterEver() && block->canEnterEver(m_actor);
		}
		auto destinationCondition = [&](Block* block)
		{
			for(Plant* plant : block->m_plants)
				if(m_actor.canEat(plant))
					return true;
			return false;
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_animal.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
			m_actor.onNothingToGraze();
		else
		{
			m_actor.setDestination(m_result);
			m_actor.m_queuedActions.emplace_back(std::make_unique<GrazeQueuedAction>(m_animal));
		}
			
	}
};
template<class Animal>
class GrazeObjective : public Objective
{
	Animal& m_animal;
	GrazeObjective(Animal& a) : Objective(Config::eatPriority), m_animal(a) {}
	void execute()
	{
		m_animal.m_location->m_area->registerGrazeRequest(m_animal);
	}
};
