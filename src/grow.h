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
	uint32_t m_percentGrown;
	uint32_t m_birthStep;
public:
	CanGrow(Actor& a, uint32_t pg);
	void updateGrowingStatus();
	uint32_t growthPercent() const;
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
	AnimalGrowthEvent(uint32_t delay, CanGrow& cg);
	void execute() { m_canGrow.complete(); }
	void clearReferences(){ m_canGrow.m_event.clearPointer(); }
};
class AnimalGrowthUpdateEvent final : public ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
public:
	AnimalGrowthUpdateEvent(uint32_t delay, CanGrow& cg);
	void execute() { m_canGrow.update(); }
	void clearReferences(){ m_canGrow.m_updateEvent.clearPointer(); }
};
