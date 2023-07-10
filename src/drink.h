#pragma once

#include "util.h"
#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"

class Item;
class Block;
class Actor;
class DrinkThreadedTask;
class DrinkEvent;
class ThirstEvent;
class DrinkObjective;

class MustDrink
{
	Actor& m_actor;
	uint32_t m_volumeDrinkRequested;
	DrinkObjective* m_objective;
	HasScheduledEventPausable<ThirstEvent> m_thirstEvent;
	uint32_t volumeFluidForBodyMass() const;

public:
	MustDrink(Actor& a);
	void drink(uint32_t volume);
	void setNeedsFluid();
	const uint32_t& getVolumeFluidRequested() const;
	const uint32_t& getPercentDeadFromThirst() const;
	friend class ThirstEvent;
};
class DrinkObjective : public Objective
{
	Actor& m_actor;
	HasThreadedTask<DrinkThreadedTask> m_threadedTask;
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
public:
	DrinkObjective(Actor& a);
	void execute();
	bool canDrinkAt(Block& block) const;
	Block* getAdjacentBlockToDrinkAt(Block& block) const;
	bool canDrinkItemAt(Block* block) const;
	Item* getItemToDrinkFromAt(Block* block) const;
};
class DrinkEvent : public ScheduledEvent
{
	DrinkObjective& m_drinkObjective;
	Item* m_item;
public:
	DrinkEvent(uint32_t step, DrinkObjective& drob);
	DrinkEvent(uint32_t step, DrinkObjective& drob, Item& i);
	void execute();
};
class DrinkThreadedTask : ThreadedTask
{
public:
	DrinkObjective& m_drinkObjective;
	std::vector<Block*> m_result;
	DrinkThreadedTask(DrinkObjective& drob);

	void readStep();
	void writeStep();
};
