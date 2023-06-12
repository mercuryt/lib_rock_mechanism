#pragma once
#include "util.h"
#include "objective.h"
#include "eventSchedule.h"

template<class EatObjective>
class EatEvent : public ScheduledEvent
{
	EatObjective& m_eatObjective;
public:
	EatEvent(step, EatObjective& eo) : ScheduledEvent(step), m_eatObjective(eo) {}
	void execute()
	{
		auto& animal = m_eatObjective.m_animal;
		if(!animal.canEatAt(*animal.m_location))
			m_eatObjective.m_threadedTask.create(m_eatObjective);
		else
		{
			if(m_animal.m_animalType.carnivore)
				for(Actor* actor : block->m_actors)
					if(!actor.m_alive && m_animal.canEat(actor))
						m_animal.eat(*actor);
			if(m_animal.m_animalType.herbavore)
				for(Plant* plant : block->m_plants)
					if(m_animal.canEat(plant))
						m_animal.eat(*plant);
			animal.finishedCurrentObjective();
		}
	}
};
template<class Block, class Animal>
class EatThreadedTask : ThreadedTask
{
	Animal& m_animal;
	EatObjective& m_eatObjective;
	Block* m_blockResult;
	Actor* m_huntResult;
public:
	EatThreadedTask(EatObjective& eo) : m_fluidType(ft), m_eatObjective(eo), m_blockResult(nullptr), m_huntResult(nullptr) {}

	void readStep()
	{
		auto destinationCondition = [&](Block* block)
		{
			m_eatObjective.canEatAt(*block);
		}
		m_blockResult = util::findWithPathCondition(pathCondition, destinationCondition, *m_eatObjective.m_animal.m_location);
		// Nothing to scavenge or graze, maybe hunt.
		if(m_blockResult == nullptr && m_animal.m_animalType.carnivore)
		{
			auto huntCondition = [&](Block* block)
			{
				for(Actor& actor : block->m_actors)
					if(m_animal.canEat(actor))
						return true;
				return false;
			}
			m_huntResult = util::findWithPathCondition(pathCondition, huntCondition, *m_eatObjective.m_animal.m_location);
		}
	}
	void writeStep()
	{
		m_eatObjective.m_threadedTask = nullptr;
		if(m_blockResult == nullptr)
		{
			if(m_huntResult == nullptr)
				m_eatObjective.m_animal.onNothingToEat();
			else
			{
				std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(Config::eatPriority + 1, *m_huntResult);
				m_animal.addObjective(std::move(killObjective));
			}
		}
		else
			m_eatObjective.m_animal.setDestination(m_result);
			
	}
};
template<class Block, class Animal>
class EatObjective : public Objective
{
public:
	Animal& m_animal;
	ThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	EatObjective(Animal& a) : Objective(Config::eatPriority), m_animal(a) {}
	void execute()
	{
		if(!canEatAt(m_animal.m_location))
			m_threadedTask.create(*this);
		else
			m_eatEvent.schedule(::s_step + Config::stepsToEat);
	}
	bool canEatAt(Block& block)
	{
		for(Block* adjacent : block.m_adjacentsVector)
		{
			if(m_animal.m_animalType.carnivore)
				for(Actor* actor : block->m_actors)
					if(!actor.m_alive && m_animal.canEat(actor))
						return true;
			if(m_animal.m_animalType.herbavore)
				for(Plant* plant : block->m_plants)
					if(m_animal.canEat(plant))
						return true;
		}
		return false;
	}
};
