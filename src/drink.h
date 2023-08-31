#pragma once

#include "objective.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"

#include <vector>

class Item;
class Block;
class Actor;
class DrinkThreadedTask;
class DrinkEvent;
class ThirstEvent;
class DrinkObjective;
struct FluidType;

class MustDrink final
{
	Actor& m_actor;
	uint32_t m_volumeDrinkRequested;
	const FluidType* m_fluidType; // Pointer because it may change, i.e. through vampirism.
	DrinkObjective* m_objective; // Store to avoid recreating. TODO: Use a bool instead?
	HasScheduledEventPausable<ThirstEvent> m_thirstEvent;
	uint32_t volumeFluidForBodyMass() const;

public:
	MustDrink(Actor& a);
	void drink(const uint32_t volume);
	void setNeedsFluid();
	const uint32_t& getVolumeFluidRequested() const { return m_volumeDrinkRequested; }
	const uint32_t& getPercentDeadFromThirst() const;
	const FluidType& getFluidType() const { return *m_fluidType; }
	bool needsFluid() const { return m_volumeDrinkRequested != 0; }
	static uint32_t drinkVolumeFor(Actor& actor);
	friend class ThirstEvent;
	friend class DrinkEvent;
	friend class DrinkObjective;
};
class DrinkObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<DrinkThreadedTask> m_threadedTask;
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
public:
	DrinkObjective(Actor& a);
	void execute();
	void cancel();
	std::string name() { return "drink"; }
	bool canDrinkAt(const Block& block) const;
	Block* getAdjacentBlockToDrinkAt(const Block& block) const;
	bool canDrinkItemAt(const Block& block) const;
	Item* getItemToDrinkFromAt(Block& block) const;
	friend class DrinkEvent;
	friend class DrinkThreadedTask;
};
class DrinkEvent final : public ScheduledEventWithPercent
{
	DrinkObjective& m_drinkObjective;
	Item* m_item;
public:
	DrinkEvent(const Step delay, DrinkObjective& drob);
	DrinkEvent(const Step delay, DrinkObjective& drob, Item& i);
	void execute();
	void clearReferences();
};
class ThirstEvent final : public ScheduledEventWithPercent
{
	Actor& m_actor;
public:
	ThirstEvent(const Step delay, Actor& a);
	void execute();
	void clearReferences();
};
class DrinkThreadedTask final : public ThreadedTask
{
	DrinkObjective& m_drinkObjective;
	std::vector<Block*> m_result;
public:
	DrinkThreadedTask(DrinkObjective& drob);
	void readStep();
	void writeStep();
	void clearReferences();
};
