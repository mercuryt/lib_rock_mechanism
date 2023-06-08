#pragma once
#include "util.h"
#include "objective.h"
#include "eventSchedule.h"

template<class Animal>
class DrinkEvent : public ScheduledEvent
{
public:
	Animal& m_animal;
	DrinkObjective& m_drinkObjective;
	DrinkEvent(uint32_t step, DrinkObjective& drob) : ScheduledEvent(step), m_drinkObjective(drob) {}
	void execute()
	{
		m_drinkObjective.m_animal.drink();
		m_animal.finishedCurrentObjective();
	}
};
template<class Block, class Animal, class FluidType>
class DrinkThreadedTask : ThreadedTask
{
public:
	Animal& m_animal;
	DrinkObjective& m_drinkObjective;
	Block* m_result;
	DrinkThreadedTask(DrinkObjective& drob) : m_fluidType(ft), m_drinkObjective(drob) {}

	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return block->anyoneCanEnterEver() && block->canEnterEver(m_drinkObjective.m_animal);
		}
		auto destinationCondition = [&](Block* block)
		{
			m_drinkObjective.canDrinkAt(*block);
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_drinkObjective.m_animal.m_location)
	}
	void writeStep()
	{
		m_drinkObjective.m_threadedTask = nullptr;
		if(m_result == nullptr)
			m_drinkObjective.m_animal.onNothingToDrink();
		else
			m_drinkObjective.m_animal.setDestination(m_result);
			
	}
};
template<class Block, class Animal>
class DrinkObjective : public Objective
{
public:
	Animal& m_animal;
	ThreadedTask<DrinkThreadedTask> m_threadedTask;
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	DrinkObjective(Animal& a) : Objective(Config::drinkPriority), m_animal(a) { }
	void execute()
	{
		if(!canDrinkAt(m_animal.m_location))
			m_threadedTask.create(*this);
		else
			m_drinkEvent.schedule(::s_step + Const::stepsToDrink, *this);
	}
	bool canDrinkAt(Block& block)
	{
		for(Block* adjacent : block.m_adjacentsVector)
			if(adjacent->m_fluids.contains(m_animal.m_animalType.fluidType))
				return true;
		return false;
	}
};
