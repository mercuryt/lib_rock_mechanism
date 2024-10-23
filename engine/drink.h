#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "reference.h"
#include "types.h"

#include <vector>

class Simulation;
class DrinkPathRequest;
class DrinkEvent;
class ThirstEvent;
class DrinkObjective;
class Area;
struct FluidType;
struct DeserializationMemo;

class MustDrink final
{
	HasScheduledEvent<ThirstEvent> m_thirstEvent; // 2
	ActorReference m_actor;
	FluidTypeId m_fluidType;
	DrinkObjective* m_objective = nullptr; // Store to avoid recreating. TODO: Use a bool instead?
	CollisionVolume m_volumeDrinkRequested = CollisionVolume::create(0);
	[[nodiscard]] CollisionVolume volumeFluidForBodyMass() const;

public:
	MustDrink(Area& area, const ActorIndex& a);
	MustDrink(Area& area, const Json& data, const ActorIndex& a, const AnimalSpeciesId& species);
	void drink(Area& area, const CollisionVolume& volume);
	void notThirsty(Area& area);
	void setNeedsFluid(Area& area);
	void unschedule();
	void scheduleDrinkEvent(Area& area);
	void setFluidType(const FluidTypeId& fluidType);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] CollisionVolume getVolumeFluidRequested() const { return m_volumeDrinkRequested; }
	[[nodiscard]] Percent getPercentDeadFromThirst() const;
	[[nodiscard]] FluidTypeId getFluidType() const { return m_fluidType; }
	[[nodiscard]] bool needsFluid() const { return m_volumeDrinkRequested != 0; }
	[[nodiscard]] static CollisionVolume drinkVolumeFor(Area& area, const ActorIndex& actor);
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
	ActorReference m_actor;
	ItemReference m_item;
public:
	DrinkEvent(Area& area, const Step& delay, DrinkObjective& drob, const ActorIndex& actor, const Step start = Step::null());
	DrinkEvent(Area& area, const Step& delay, DrinkObjective& drob, const ActorIndex& actor, const ItemIndex& i, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class ThirstEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	ThirstEvent(Area& area, const Step& delay, const ActorIndex& a, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
