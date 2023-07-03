#pragma once
#include "actor.h"
#include "eventSchedule.h"

class CanGrow
{
	Actor& m_actor;
	HasScheduledEvent<AnimalGrowthEvent> m_event;
	HasScheduledEvent<AnimalGrowthUpdateEvent> m_updateEvent;
	uint32_t m_percentGrown;
	CanGrow(Actor& a, uint32_t pg);
	void updateGrowingStatus();
	uint32_t growthPercent() const;
	void update();
};
class AnimalGrowthEvent : ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
	AnimalGrowthEvent(uint32_t step, CanGrow& cg) : ScheduledEventWithPercent(step), m_canGrow(cg) { }
	void execute()
	{
		m_canGrow.complete();
	}
	~AnimalGrowthEvent(){ m_canGrow.m_event.clearPointer(); }
};
class AnimalGrowthUpdateEvent : ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
	AnimalGrowthEvent(uint32_t step, CanGrow& cg) : ScheduledEventWithPercent(step), m_canGrow(cg) { }
	void execute()
	{
		m_canGrow.update();
	}
	~AnimalGrowthEvent(){ m_canGrow.m_updateEvent.clearPointer(); }
};
