#pragma once
#include "util.h"
#include "objective.h"
#include "eventSchedule.h"
#include "path.h"

class DrinkEvent : public ScheduledEvent
{
public:
	Actor& m_actor;
	DrinkObjective& m_drinkObjective;
	Item* m_item;
	DrinkEvent(uint32_t step, DrinkObjective& drob) : ScheduledEvent(step), m_drinkObjective(drob), m_item(nullptr) {}
	DrinkEvent(uint32_t step, DrinkObjective& drob, Item& i) : ScheduledEvent(step), m_drinkObjective(drob), m_item(i) {}
	void execute()
	{
		//TODO: What if not enough volume is avalible?
		if(m_item != nullptr)
		{
			if(m_item.m_containsItemsOrFluids.m_fluidType != m_actor.getFluidType())
			{
				m_drinkObjective.m_threadedTask.create(m_drinkObjective);
				return;
			}
			m_item.m_containsItemsOrFluids.remove(m_actor.getFluidType(), m_actor.getFluidDrinkVolume());
			m_item.m_reservable.unreserveFor(m_drinkObjective.m_actor.m_canReserve);
		}
		else
		{
			Block* drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(*m_actor.m_location);
			if(drinkBlock == nullptr)
			{
				m_drinkObjective.m_threadedTask.create(m_drinkObjective);
				return;
			}
			drinkBlock->m_hasFluids.remove(m_actor.getFluidType(), m_actor.getFluidDrinkVolume())
		}
		m_drinkObjective.m_actor.drink();
		m_actor.finishedCurrentObjective();
	}
};
template<class Block, class FluidType>
class DrinkThreadedTask : ThreadedTask
{
public:
	DrinkObjective& m_drinkObjective;
	std::vector<Block*> m_result;
	DrinkThreadedTask(DrinkObjective& drob) : m_drinkObjective(drob) {}

	void readStep()
	{
		auto destinationCondition = [&](Block* block)
		{
			return m_drinkObjective.canDrinkItemAt(*block) || m_drinkObjective.canDrinkAt(*block);
		}
		m_result = path::getForActorToPredicate(m_drinkObjective.m_animal, destinationCondition);
	}
	void writeStep()
	{
		if(m_result.empty())
			m_drinkObjective.m_actor.m_publisher.publish(PublishedEventType::NothingToDrink);
		else
			m_drinkObjective.m_actor.setPath(m_result);
	}
	~DrinkThreadedTask(){ m_drinkObjective.m_threadedTask.clearPointer(); }
};
template<class Block, class Actor>
class DrinkObjective : public Objective
{
public:
	Actor& m_actor;
	ThreadedTask<DrinkThreadedTask> m_threadedTask;
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	DrinkObjective(Actor& a) : Objective(Config::drinkPriority), m_actor(a) { }
	void execute()
	{
		Item* item = getItemToDrinkFromAt(*m_actor.m_location);
		if(item != nullptr)
		{
			item.m_reservable.reserveFor(m_actor.m_canReserve, 1);
			m_drinkEvent.schedule(::s_step + Config::stepsToDrink, *this, item);
		}
		else
		{
			if(!canDrinkAt(*m_actor.m_location))
				m_threadedTask.create(*this);
			else
				m_drinkEvent.schedule(::s_step + Config::stepsToDrink, *this);
		}
	}
	bool canDrinkAt(Block& block) const
	{
		return getAdjacentBlockToDrinkAt(block) != nullptr;
	}
	Block* getAdjacentBlockToDrinkAt(Block& block) const
	{
		for(Block* adjacent : block.m_adjacentsVector)
			if(adjacent->m_fluids.contains(m_actor.getFluidType()))
				return adjacent;
		return nullptr;
	}
	bool canDrinkItemAt(Block* block) const
	{
		return getItemToDrinkFromAt(block) != nullptr;
	}
	Item* getItemToDrinkFromAt(Block* block) const
	{
		assert(m_actor.getFluidType() != nullptr);
		for(Item* item : block.m_hasItems.get())
			if(item->containsItemsOrFluids.getFluidType() == m_actor.getFluidType())
				return &item;
	}
};
