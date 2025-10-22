#pragma once

#include "config.h"
#include "reference.h"
#include "simulation/simulation.h"
#include "eventSchedule.hpp"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"

class EatObjective;
class HungerEvent;
struct DeserializationMemo;
struct AnimalSpecies;

class MustEat final
{
	HasScheduledEvent<HungerEvent> m_hungerEvent; // 2
	ActorReference m_actor;
	EatObjective* m_eatObjective = nullptr;
public:
	Point3D m_eatingLocation;
private:
	Mass m_massFoodRequested = Mass::create(0);
public:
	MustEat(Area& area, const ActorIndex& a);
	MustEat(Area& area, const Json& data, const ActorIndex& a, AnimalSpeciesId species);
	[[nodiscard]]Json toJson() const;
	void scheduleHungerEvent(Area& area);
	void eat(Area& area, Mass mass);
	void notHungry(Area& area);
	void setNeedsFood(Area& area);
	void unschedule();
	void setObjective(EatObjective& objective);
	void setPercentStarved(const Percent& percent);
	[[nodiscard]] bool needsFood() const;
	[[nodiscard]] Mass massFoodForBodyMass(Area& area) const;
	[[nodiscard]] Mass getMassFoodRequested() const;
	[[nodiscard]] Percent getPercentStarved() const;
	[[nodiscard]] std::pair<Point3D, uint8_t> getDesireToEatSomethingAt(Area& area, const Cuboid& cuboid) const;
	[[nodiscard]] uint8_t getMinimumAcceptableDesire(Area& area) const;
	[[nodiscard]] Point3D getOccupiedOrAdjacentPointWithHighestDesireFoodOfAcceptableDesireability(Area& area);
	[[nodiscard]] bool canEatActor(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] bool canEatPlant(Area& area, const PlantIndex& plant) const;
	[[nodiscard]] bool canEatItem(Area& area, const ItemIndex& item) const;
	friend class HungerEvent;
	friend class EatObjective;
	// For testing.
	[[maybe_unused]] bool hasObjective() const { return m_eatObjective != nullptr; }
	[[maybe_unused]] bool hasHungerEvent() const { return m_hungerEvent.exists(); }
	[[maybe_unused]] Step getHungerEventStep() const { return m_hungerEvent.getStep(); }
};
class HungerEvent final : public ScheduledEvent
{
	ActorIndex m_actor;
public:
	HungerEvent(Area& area, const Step& delay, const ActorIndex& a, const Step start = Step::null());
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
};
