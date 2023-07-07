#pragma once
#include "actor.h"
#include "eventSchedule.h"
class AnimalGrowthEvent;
class AnimalGrowthUpdateEvent;
class CanGrow
{
	Actor& m_actor;
	HasScheduledEvent<AnimalGrowthEvent> m_event;
	HasScheduledEvent<AnimalGrowthUpdateEvent> m_updateEvent;
	uint32_t m_percentGrown;
public:
	CanGrow(Actor& a, uint32_t pg);
	void updateGrowingStatus();
	uint32_t growthPercent() const;
	void update();
	void complete();
	friend class AnimalGrowthEvent;
	friend class AnimalGrowthUpdateEvent;
};
class AnimalGrowthEvent : public ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
public:
	AnimalGrowthEvent(uint32_t delay, CanGrow& cg) : ScheduledEventWithPercent(delay), m_canGrow(cg) { }
	void execute()
	{
		m_canGrow.complete();
	}
	~AnimalGrowthEvent(){ m_canGrow.m_event.clearPointer(); }
};
class AnimalGrowthUpdateEvent : public ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
public:
	AnimalGrowthUpdateEvent(uint32_t delay, CanGrow& cg) : ScheduledEventWithPercent(delay), m_canGrow(cg) { }
	void execute()
	{
		m_canGrow.update();
	}
	~AnimalGrowthUpdateEvent(){ m_canGrow.m_updateEvent.clearPointer(); }
};
