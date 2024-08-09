#pragma once
#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../types.h"
#include "simulation.h"
#include <vector>
class Area;
struct AnimalSpecies;
class AnimalsLeaveScheduledEvent;
struct DeserializationMemo;

struct AnimalsArriveDramaArc final : public DramaArc
{
	BlockIndex m_entranceBlock;
	ActorReferences m_actors;
	bool m_isActive = false;
	AnimalSpeciesId m_species;
	Quantity m_quantity;
	Percent m_hungerPercent;
	Percent m_thristPercent;
	Percent m_tiredPercent;
	AnimalsArriveDramaArc(DramaEngine& engine, Area& area);
	AnimalsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	void callback();
	HasScheduledEvent<AnimalsLeaveScheduledEvent> m_scheduledEvent;
private:
	void scheduleArrive();
	void scheduleDepart();
	void scheduleContinue();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::pair<AnimalSpeciesId, Quantity> getSpeciesAndQuantity() const;
	[[nodiscard]] static std::vector<AnimalSpeciesId> getLargeCarnivors();
	[[nodiscard]] static std::vector<AnimalSpeciesId> getMediumCarnivors();
	[[nodiscard]] static std::vector<AnimalSpeciesId> getSmallCarnivors();
	[[nodiscard]] static std::vector<AnimalSpeciesId> getLargeHerbivors();
	[[nodiscard]] static std::vector<AnimalSpeciesId> getMediumHerbivors();
	[[nodiscard]] static std::vector<AnimalSpeciesId> getSmallHerbivors();
	[[nodiscard]] static bool isSmall(ShapeId shape);
	[[nodiscard]] static bool isMedium(ShapeId shape);
	[[nodiscard]] static bool isLarge(ShapeId shape);
	//For debug.
	void begin();
};
class AnimalsLeaveScheduledEvent final : public ScheduledEvent
{
	AnimalsArriveDramaArc& m_dramaticArc;
public:
	AnimalsLeaveScheduledEvent(AnimalsArriveDramaArc& event, Simulation& simulation, Step duration, Step start = Step::create(0)) : ScheduledEvent(simulation, duration, start), m_dramaticArc(event) { }
	void execute(Simulation&, Area*) { m_dramaticArc.callback(); }
	void clearReferences(Simulation&, Area*) { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};
