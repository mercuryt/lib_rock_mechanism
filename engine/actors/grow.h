#pragma once
#include "config.h"
#include "eventSchedule.hpp"
class Simulation;
class AnimalGrowthEvent;
class Actor;
class CanGrow final
{
	HasScheduledEventPausable<AnimalGrowthEvent> m_event;
	ActorIndex m_actor;
	Percent m_percentGrown;
public:
	CanGrow(Area& area, ActorIndex a, Percent pg);
	CanGrow(const Json& data, ActorIndex actor, Simulation& s);
	[[nodiscard]] Json toJson() const;
	void updateGrowingStatus(Area& area);
	void setGrowthPercent(Area& area, Percent percent);
	void scheduleEvent(Area& area);
	void update(Area& area);
	void stop();
	void maybeStart(Area& area);
	void increment(Area& area);
	[[nodiscard]] bool canGrowCurrently(Area& area) const;
	[[nodiscard]] Percent growthPercent() const;
	[[nodiscard]] bool isGrowing() const { return !m_event.isPaused(); }
	friend class AnimalGrowthEvent;
	// For test.
	[[maybe_unused, nodiscard]] Percent getEventPercentComplete() { return m_event.percentComplete(); }
	[[maybe_unused, nodiscard]] HasScheduledEventPausable<AnimalGrowthEvent>& getEvent() { return m_event; }
	[[maybe_unused, nodiscard]] bool eventEvents() { return m_event.exists(); }
};
class AnimalGrowthEvent final : public ScheduledEvent
{
	CanGrow& m_canGrow;
public:
	AnimalGrowthEvent(Area& area, Step delay, CanGrow& cg, const Step start = 0);
	void execute(Simulation&, Area* area) { m_canGrow.increment(*area); }
	void clearReferences(Simulation&, Area*){ m_canGrow.m_event.clearPointer(); }
};
