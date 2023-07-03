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
	DrinkEvent(uint32_t step, DrinkObjective& drob);
	DrinkEvent(uint32_t step, DrinkObjective& drob, Item& i);
	void execute();
};
template<class Block, class FluidType>
class DrinkThreadedTask : ThreadedTask
{
public:
	DrinkObjective& m_drinkObjective;
	std::vector<Block*> m_result;
	DrinkThreadedTask(DrinkObjective& drob);

	void readStep();
	void writeStep();
};
template<class Block, class Actor>
class DrinkObjective : public Objective
{
public:
	Actor& m_actor;
	ThreadedTask<DrinkThreadedTask> m_threadedTask;
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	DrinkObjective(Actor& a);
	void execute();
	bool canDrinkAt(Block& block) const;
	Block* getAdjacentBlockToDrinkAt(Block& block) const;
	bool canDrinkItemAt(Block* block) const;
	Item* getItemToDrinkFromAt(Block* block) const;
};
