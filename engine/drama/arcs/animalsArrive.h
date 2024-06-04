#pragma once
#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../types.h"
#include <vector>
class Area;
class Actor;
struct AnimalSpecies;
class AnimalsLeaveScheduledEvent;
struct DeserializationMemo;

struct AnimalsArriveDramaArc final : public DramaArc
{
	BlockIndex m_entranceBlock = BLOCK_INDEX_MAX;
	std::vector<Actor*> m_actors;
	bool m_isActive = false;
	const AnimalSpecies* m_species = nullptr;
	uint32_t m_quantity = 0;
	Percent m_hungerPercent = 0;
	Percent m_thristPercent = 0;
	Percent m_tiredPercent = 0;
	AnimalsArriveDramaArc(DramaEngine& engine, Area& area);
	AnimalsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	[[nodiscard]] Json toJson() const;
	void callback();
	HasScheduledEvent<AnimalsLeaveScheduledEvent> m_scheduledEvent;
private:
	void scheduleArrive();
	void scheduleDepart();
	void scheduleContinue();
	[[nodiscard]] std::pair<const AnimalSpecies*, uint32_t> getSpeciesAndQuantity() const;
	[[nodiscard]] static std::vector<const AnimalSpecies*> getLargeCarnivors();
	[[nodiscard]] static std::vector<const AnimalSpecies*> getMediumCarnivors();
	[[nodiscard]] static std::vector<const AnimalSpecies*> getSmallCarnivors();
	[[nodiscard]] static std::vector<const AnimalSpecies*> getLargeHerbivors();
	[[nodiscard]] static std::vector<const AnimalSpecies*> getMediumHerbivors();
	[[nodiscard]] static std::vector<const AnimalSpecies*> getSmallHerbivors();
	[[nodiscard]] static bool isSmall(const Shape& shape);
	[[nodiscard]] static bool isMedium(const Shape& shape);
	[[nodiscard]] static bool isLarge(const Shape& shape);
	//For debug.
	void begin();
};
class AnimalsLeaveScheduledEvent final : public ScheduledEvent
{
	AnimalsArriveDramaArc& m_dramaticArc;
public:
	AnimalsLeaveScheduledEvent(AnimalsArriveDramaArc& event, Simulation& simulation, Step duration, Step start = 0) : ScheduledEvent(simulation, duration, start), m_dramaticArc(event) { }
	void execute() { m_dramaticArc.callback(); }
	void clearReferences() { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};
