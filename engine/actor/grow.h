#pragma once
#include "config.h"
#include "eventSchedule.h"
#include "eventSchedule.hpp"
class Simulation;
class AnimalGrowthEvent;
class Actor;
class CanGrow final
{
	HasScheduledEventPausable<AnimalGrowthEvent> m_event;
	Actor& m_actor;
	Percent m_percentGrown;
public:
	CanGrow(Actor& a, Percent pg, Simulation& s);
	CanGrow(const Json& data, Actor& actor, Simulation& s);
	[[nodiscard]] Json toJson() const;
	void updateGrowingStatus();
	[[nodiscard]] bool canGrowCurrently() const;
	void setGrowthPercent(Percent percent);
	[[nodiscard]] Percent growthPercent() const;
	void scheduleEvent();
	void update();
	void stop();
	void maybeStart();
	void increment();
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
	AnimalGrowthEvent(Step delay, CanGrow& cg, const Step start = 0);
	void execute() { m_canGrow.increment(); }
	void clearReferences(){ m_canGrow.m_event.clearPointer(); }
};
