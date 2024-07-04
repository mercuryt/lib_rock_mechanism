#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "types.h"
#include "findsPath.h"

#include <vector>

class Simulation;
class DrinkPathRequest;
class DrinkEvent;
class ThirstEvent;
class DrinkObjective;
class Area;
struct FluidType;
struct DeserializationMemo;
struct AnimalSpecies;

class MustDrink final
{
	HasScheduledEvent<ThirstEvent> m_thirstEvent; // 2
	ActorIndex m_actor = ACTOR_INDEX_MAX;
	const FluidType* m_fluidType = nullptr; // Pointer because it may change, i.e. through vampirism.
	DrinkObjective* m_objective = nullptr; // Store to avoid recreating. TODO: Use a bool instead?
	uint32_t m_volumeDrinkRequested = 0;
	[[nodiscard]] uint32_t volumeFluidForBodyMass() const;

public:
	MustDrink(Area& area, ActorIndex a);
	MustDrink(const Json& data, ActorIndex a, Simulation& s, const AnimalSpecies& species);
	[[nodiscard]] Json toJson() const;
	void drink(Area& area, const uint32_t volume);
	void notThirsty(Area& area);
	void setNeedsFluid(Area& area);
	void onDeath();
	void scheduleDrinkEvent(Area& area);
	void setFluidType(const FluidType& fluidType);
	[[nodiscard]] Volume getVolumeFluidRequested() const { return m_volumeDrinkRequested; }
	[[nodiscard]] Percent getPercentDeadFromThirst() const;
	[[nodiscard]] const FluidType& getFluidType() const { return *m_fluidType; }
	[[nodiscard]] bool needsFluid() const { return m_volumeDrinkRequested != 0; }
	[[nodiscard]] static uint32_t drinkVolumeFor(Area& area, ActorIndex actor);
	friend class ThirstEvent;
	friend class DrinkEvent;
	friend class DrinkObjective;
	// For testing.
	[[maybe_unused]] bool thirstEventExists() const { return m_thirstEvent.exists(); }
	[[maybe_unused]] bool objectiveExists() const { return m_objective != nullptr; }
};
class DrinkEvent final : public ScheduledEvent
{
	DrinkObjective& m_drinkObjective;
	ItemIndex m_item = ITEM_INDEX_MAX;
public:
	DrinkEvent(Area& area, const Step delay, DrinkObjective& drob, const Step start = 0);
	DrinkEvent(Area& area, const Step delay, DrinkObjective& drob, ItemIndex i, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class ThirstEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	ThirstEvent(Area& area, const Step delay, ActorIndex a, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
;
