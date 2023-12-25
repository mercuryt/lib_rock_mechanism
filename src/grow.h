#pragma once
#include "eventSchedule.h"
#include "eventSchedule.hpp"
class AnimalGrowthEvent;
class AnimalGrowthUpdateEvent;
class Actor;
class CanGrow final
{
	Actor& m_actor;
	HasScheduledEvent<AnimalGrowthEvent> m_event;
	HasScheduledEvent<AnimalGrowthUpdateEvent> m_updateEvent;
	Percent m_percentGrown;
public:
	CanGrow(Actor& a, Percent pg);
	CanGrow(const Json& data);
	Json toJSon() const;
	void updateGrowingStatus();
	Percent growthPercent() const;
	void update();
	void complete();
	void stop();
	void maybeStart();
	bool isGrowing() const { return m_event.exists(); }
	friend class AnimalGrowthEvent;
	friend class AnimalGrowthUpdateEvent;
};
class AnimalGrowthEvent final : public ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
public:
	AnimalGrowthEvent(Step delay, CanGrow& cg);
	void execute() { m_canGrow.complete(); }
	void clearReferences(){ m_canGrow.m_event.clearPointer(); }
};
class AnimalGrowthUpdateEvent final : public ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
public:
	AnimalGrowthUpdateEvent(Step delay, CanGrow& cg);
	void execute() { m_canGrow.update(); }
	void clearReferences(){ m_canGrow.m_updateEvent.clearPointer(); }
};
