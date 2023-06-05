#pragma once
#include "util.h"
#include "queuedAction.h"
#include "eventSchedule.h"

template<class Animal>
class DrinkEvent : public ScheduledEvent
{
	Animal& m_animal;
	DrinkEvent(Animal& a) : ScheduledEvent(s_step + Const::stepsToDrink), m_animal(a) {}
	void execute()
	{
		m_animal.drink();
		m_animal.finishedCurrentTask();
	}
	void cancel()
	{
		m_animal.m_location->m_area->registerDrinkRequest(m_animal);
		ScheduledEvent::cancel();
	}
};
template<class Animal>
class DrinkQueuedAction : public QueuedAction
{
public:
	Animal& m_animal;
	DrinkQueuedAction(Animal& a) : m_animal(a) {}
	void execute()
	{
		// If next to fluid of correct type then schedule drink event.
		for(Block* block : m_animal.m_location->m_adjacentsVector)
			if(block.m_fluids.contains(&m_fluidType))
			{
				m_animal.m_location->m_area.m_eventSchedule.schedule(std::make_unique<DrinkEvent>(m_animal));
				return;
			}
		// Otherwise register drink request.
		m_animal.m_location->m_area->registerDrinkRequest(m_animal);
	}
};
template<class Block, class Animal, class FluidType>
class DrinkRequest
{
	Animal& m_animal;
	const FluidType& m_fluidType;
	Block* m_result;
	uint32_t m_maxRange;
	DrinkRequest(Animal& a, const FluidType& ft,  uint32_t mr) : m_animal(a), m_fluidType(ft), m_maxRange(mr) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return m_animal.m_location->taxiDistance(*block) < m_maxRange && block>anyoneCanEnterEver() && block->canEnterEver(m_animal);
		}
		auto destinationCondition = [&](Block* block)
		{
			for(Block* adjacent : block.m_adjacentsVector)
				if(block.m_fluids.contains(&m_fluidType))
					return true;
			return false;
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_animal.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
			m_animal.onNothingToDrink();
		else
		{
			m_animal.setDestination(m_result);
			m_animal.m_queuedActions.emplace_back(std::make_unique<DrinkQueuedAction>(m_animal));
		}
			
	}
};
template<class Animal>
class DrinkObjective : public Objective
{
	Animal& m_animal;
	DrinkObjective(Animal& a) : Objective(Config::drinkPriority), m_animal(a) {}
	void execute()
	{
		m_animal.m_location->m_area->registerDrinkRequest(m_animal);
	}
}
