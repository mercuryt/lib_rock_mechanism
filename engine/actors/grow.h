#pragma once
#include "config.h"
#include "eventSchedule.hpp"
#include "reference.h"
class Simulation;
class AnimalGrowthEvent;
class Actor;
class CanGrow final
{
	HasScheduledEventPausable<AnimalGrowthEvent> m_event;
	ActorReference m_actor;
	Percent m_percentGrown;
public:
	CanGrow(Area& area, ActorIndex a, Percent pg);
	CanGrow(Area& area, const Json& data, ActorIndex actor);
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
	// Most events take area as their first argument but events which have resume called on them must take delay as first argument.
	// TODO: Make all events take delay as first argument.
	AnimalGrowthEvent(Step delay, Area& area, CanGrow& cg, Step start = 0);
	void execute(Simulation&, Area* area) { m_canGrow.increment(*area); }
	void clearReferences(Simulation&, Area*){ m_canGrow.m_event.clearPointer(); }
};
