#pragma once

#include "config.h"
#include "reference.h"
#include "simulation.h"
#include "eventSchedule.hpp"
#include "terrainFacade.h"
#include "types.h"

class EatObjective;
class HungerEvent;
class Plant;
struct DeserializationMemo;
struct AnimalSpecies;

class MustEat final
{
	HasScheduledEvent<HungerEvent> m_hungerEvent; // 2
	ActorReference m_actor;
	EatObjective* m_eatObjective = nullptr;
public:
	BlockIndex m_eatingLocation;
private:
	Mass m_massFoodRequested = Mass::create(0);
public:
	MustEat(Area& area, ActorIndex a);
	MustEat(Area& area, const Json& data, ActorIndex a, AnimalSpeciesId species);
	[[nodiscard]]Json toJson() const;
	void scheduleHungerEvent(Area& area);
	void eat(Area& area, Mass mass);
	void notHungry(Area& area);
	void setNeedsFood(Area& area);
	void unschedule();
	[[nodiscard]] bool needsFood() const;
	[[nodiscard]] Mass massFoodForBodyMass(Area& area) const;
	[[nodiscard]] Mass getMassFoodRequested() const;
	[[nodiscard]] Percent getPercentStarved() const;
	[[nodiscard]] uint32_t getDesireToEatSomethingAt(Area& area, BlockIndex block) const;
	[[nodiscard]] uint32_t getMinimumAcceptableDesire() const;
	[[nodiscard]] BlockIndex getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability(Area& area);
	[[nodiscard]] bool canEatActor(Area& area, const ActorIndex actor) const;
	[[nodiscard]] bool canEatPlant(Area& area, const PlantIndex plant) const;
	[[nodiscard]] bool canEatItem(Area& area, const ItemIndex item) const;
	friend class HungerEvent;
	friend class EatObjective;
	// For testing.
	[[maybe_unused]]bool hasObjecive() const { return m_eatObjective != nullptr; }
	[[maybe_unused]]bool hasHungerEvent() const { return m_hungerEvent.exists(); }
	[[maybe_unused]]Step getHungerEventStep() const { return m_hungerEvent.getStep(); }
};
class HungerEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	HungerEvent(Area& area, const Step delay, ActorIndex a, const Step start = Step::null());
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
};
