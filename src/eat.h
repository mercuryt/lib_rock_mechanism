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
class EatThreadedTask : ThreadedTask
{
	EatObjective& m_eatObjective;
	Block* m_blockResult;
	Actor* m_huntResult;
public:
	EatThreadedTask(EatObjective& eo) : m_fluidType(ft), m_eatObjective(eo), m_blockResult(nullptr), m_huntResult(nullptr) {}

	void readStep()
	{
		constexpr maxRankedEatDesire = 3;
		std::array<Block*, maxRankedEatDesire> candidates = {};
		auto destinationCondition = [&](Block* block)
		{
			uint32_t eatDesire = m_eatObjective.m_actor.m_mustEat.getDesireToEatSomethingAt(*block);
			if(eatDesire == UINT32_MAX)
				return true;
			--eatDesire;
			if(candidates[eatDesire] == nullptr)
				candidates[eatDesire] = block;
			return false;
		}
		m_blockResult = util::getForActorToPredicateReturnEndOnly(pathCondition, destinationCondition, *m_eatObjective.m_animal.m_location, Config::maxBlocksToLookForBetterFood);
		if(m_blockResult == nullptr)
			for(size_t i = 0; i < maxRankedEatDesire; ++i)
				if(candidates[i] != nullptr)
				{
					m_blockResult = candidates[i];
					continue;
				}
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
			m_huntResult = util::getForActorToPredicateReturnEndOnly(pathCondition, huntCondition, *m_eatObjective.m_animal.m_location);
		}
	}
	void writeStep()
	{
		if(m_blockResult == nullptr)
		{
			if(m_huntResult == nullptr)
				m_eatObjective.m_actor.onNothingToEat();
			else
			{
				std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(Config::eatPriority + 1, *m_huntResult);
				m_eatObjective.m_actor.addObjective(std::move(killObjective));
			}
		}
		else
		{
			m_eatObjective.m_actor.setDestination(m_result);
			//TODO: reserve food if sentient.
		}
		
			
	}
	~EatThreadedTask() { m_eatObjective.m_threadedTask.clearPointer(); }
};
class EatObjective : public Objective
{
public:
	Actor& m_actor;
	ThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	Block* m_foodLocation;
	Item* m_foodItem;
	EatObjective(Actor& a) : Objective(Config::eatPriority), m_actor(a), m_foodLocation(nullptr), m_foodItem(nullptr) { }
	void execute()
	{
		if(m_foodLocation == nullptr)
			m_threadedTask.create(*this);
		else
		{	
			Block* eatingLocation = m_actor.m_mustEat.m_eatingLocation; 
			// Civilized eating.
			if(m_actor.isSentient() && eatingLocation != nullptr && m_foodItem != nullptr)
			{
				// Has meal
				if(m_actor.m_hasItems.isCarrying(m_foodItem))
				{
					// Is at eating location.
					if(m_actor.m_location == eatingLocation)
						m_eatEvent.schedule(::s_step + Config::stepsToEat, m_foodItem);
					else
						m_actor.setDestination(eatingLocation);
				}
				else
				{
					// Is at pickup location.
					if(m_actor.m_location == m_foodLocation)
					{
						m_actor.m_hasItems.pickUp(m_foodItem);
						execute();
					}
					else
						m_actor.setDestination(m_foodLocation);
				}

			}
			// Uncivilized eating.
			else
			{
				if(m_actor.m_location == m_foodLocation)
				{
					if(m_foodItem != nullptr)
						m_eatEvent.schedule(::s_step + Config::stepsToEat, m_foodItem);
					else
						m_eatEvent.schedule(::s_step + Config::stepsToEat);
				}
				else
					m_actor.setDestination(m_foodLocation);
			}
		}
	}
	bool canEatAt(Block& block)
	{
		for(Item* item : m_block.m_hasItems.get())
		{
			if(m_actor.m_mustEat.canEat(item))
				return true;
			if(item->itemType.internalVolume != 0)
				for(Item* i : item->containsItemsOrFluids.getItems())
					if(m_actor.m_mustEat.canEat(i))
						return true;
		}
		if(m_actor.m_animalType.carnivore)
			for(Actor* other : block->m_actors)
				if(!other.m_alive && m_actor.canEat(actor))
					return true;
		if(m_actor.m_animalType.herbavore)
			for(Plant* plant : block->m_plants)
				if(m_actor.canEat(plant))
					return true;
		return false;
	}
};
class MustEat
{
	Actor& m_actor;
	uint32_t m_massFoodRequested;
	ScheduledEventWithPercent<HungerEvent> m_hungerEvent;
	const uint32_t& m_stepsNeedsFoodFrequency;
	const uint32_t& m_stepsTillDieWithoutFood;
public:
	MustEat(Actor& a, const uint32_t& snff, const uint32_t& stdwf) : m_actor(a), m_massFoodRequested(0), m_stepsNeedsFoodFrequency(snff), m_stepsTillDieWithoutFood(stdwf) { }
	void eat(uint32_t mass)
	{
		assert(mass <= m_massFoodRequested);
		m_massFoodRequested -= mass;
		m_hungerEvent.unschedule();
		if(m_massFoodRequested == 0)
			m_actor.m_growing.maybeStart();
		else
		{
			uint32_t delay = util::scaleByInverseFraction(m_stepsTillDieWithoutFood, m_massFoodRequested, massFoodForBodyMass());
			makeHungerEvent(delay);
			m_hasObjectives.addNeed(std::make_unique<EatObjective>(*this));
		}
	}
	void setNeedsFood()
	{
		if(!m_hungerEvent.empty())
			m_actor.die(CauseOfDeath("hunger"));
		else
		{
			makeHungerEvent(m_stepsTillDieWithoutFood);
			m_actor.m_growing.stop();
			m_massFoodRequested = massFoodForBodyMass();
			createFoodRequest();
		}
	}
	uint32_t massFoodForBodyMass() const
	{
		return m_actor.getMass() / Config::unitsOfBodyMassPerUnitOfFoodRequestedMass;
	}
	const uint32_t& getMassFoodRequested() const { return m_massFoodRequested; }
	const uint32_t& getPercentStarved() const
	{
		if(m_hungerEvent.empty())
			return 0;
		else
			return m_hungerEvent.percentComplete();
	}
	uint32_t getDesireToEatSomethingAt(Block* block) const
	{
		for(Item* item : block->m_hasItems.get())
			if(item.m_isPreparedMeal)
				return UINT32_MAX;
		if(m_actor.grazesFruit())
			for(Plant* plant : block->m_hasPlants.get())
				if(plant->m_percentFruit != 0)
					return 1;
		if(m_actor.scavengesCarcasses())
			for(Actor* actor : block->m_containsActors.get())
				if(!actor->m_isAlive && actor.getFluidType() == m_actor.getFluidType())
					return 2;
		if(m_actor.grazesLeaves())
			for(Plant* plant : block->m_hasPlants.get())
				if(plant->m_percentFoliage != 0)
					return 3;
		return 0;
	}
};
