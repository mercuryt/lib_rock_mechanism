#pragma once

#include "config.h"
#include "objective.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "types.h"
#include "findsPath.h"

#include <vector>

class Simulation;
class Item;
class Actor;
class DrinkThreadedTask;
class DrinkEvent;
class ThirstEvent;
class DrinkObjective;
struct FluidType;
struct DeserializationMemo;
struct AnimalSpecies;

class MustDrink final
{
	HasScheduledEvent<ThirstEvent> m_thirstEvent; // 2
	Actor& m_actor;
	const FluidType* m_fluidType = nullptr; // Pointer because it may change, i.e. through vampirism.
	DrinkObjective* m_objective = nullptr; // Store to avoid recreating. TODO: Use a bool instead?
	uint32_t m_volumeDrinkRequested = 0;
	[[nodiscard]] uint32_t volumeFluidForBodyMass() const;

public:
	MustDrink(Actor& a, Simulation& s);
	MustDrink(const Json& data, Actor& a, Simulation& s, const AnimalSpecies& species);
	[[nodiscard]] Json toJson() const;
	void drink(const uint32_t volume);
	void notThirsty();
	void setNeedsFluid();
	void onDeath();
	void scheduleDrinkEvent();
	void setFluidType(const FluidType& fluidType);
	[[nodiscard]] Volume getVolumeFluidRequested() const { return m_volumeDrinkRequested; }
	[[nodiscard]] Percent getPercentDeadFromThirst() const;
	[[nodiscard]] const FluidType& getFluidType() const { return *m_fluidType; }
	[[nodiscard]] bool needsFluid() const { return m_volumeDrinkRequested != 0; }
	[[nodiscard]] static uint32_t drinkVolumeFor(Actor& actor);
	friend class ThirstEvent;
	friend class DrinkEvent;
	friend class DrinkObjective;
	// For testing.
	[[maybe_unused]] bool thirstEventExists() const { return m_thirstEvent.exists(); }
	[[maybe_unused]] bool objectiveExists() const { return m_objective != nullptr; }
};
class DrinkObjective final : public Objective
{
	HasThreadedTask<DrinkThreadedTask> m_threadedTask;
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	bool m_noDrinkFound;
public:
	DrinkObjective(Actor& a);
	DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute();
	void cancel();
	void delay();
	void reset();
	[[nodiscard]] std::string name() const { return "drink"; }
	[[nodiscard]] bool canDrinkAt(const BlockIndex block, Facing facing) const;
	[[nodiscard]] BlockIndex getAdjacentBlockToDrinkAt(BlockIndex block, Facing facing) const;
	[[nodiscard]] bool canDrinkItemAt(BlockIndex block) const;
	[[nodiscard]] Item* getItemToDrinkFromAt(BlockIndex block) const;
	[[nodiscard]] bool containsSomethingDrinkable(BlockIndex block) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Drink; }
	[[nodiscard]] bool isNeed() const { return true; }
	friend class DrinkEvent;
	friend class DrinkThreadedTask;
};
class DrinkEvent final : public ScheduledEvent
{
	DrinkObjective& m_drinkObjective;
	Item* m_item;
public:
	DrinkEvent(const Step delay, DrinkObjective& drob, const Step start = 0);
	DrinkEvent(const Step delay, DrinkObjective& drob, Item& i, const Step start = 0);
	void execute();
	void clearReferences();
};
class ThirstEvent final : public ScheduledEvent
{
	Actor& m_actor;
public:
	ThirstEvent(const Step delay, Actor& a, const Step start = 0);
	void execute();
	void clearReferences();
};
class DrinkThreadedTask final : public ThreadedTask
{
	DrinkObjective& m_drinkObjective;
	FindsPath m_findsPath;
	bool m_noDrinkFound;
public:
	DrinkThreadedTask(DrinkObjective& drob);
	void readStep();
	void writeStep();
	void clearReferences();
};
