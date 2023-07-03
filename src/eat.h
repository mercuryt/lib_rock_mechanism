#pragma once
#include "util.h"
#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "actor.h"

class MustEat
{
	Actor& m_actor;
	uint32_t m_massFoodRequested;
	ScheduledEventWithPercent<HungerEvent> m_hungerEvent;
	const uint32_t& m_stepsNeedsFoodFrequency;
	const uint32_t& m_stepsTillDieWithoutFood;
public:
	MustEat(Actor& a, const uint32_t& snff, const uint32_t& stdwf);
	void eat(uint32_t mass);
	void setNeedsFood();
	uint32_t massFoodForBodyMass() const;
	const uint32_t& getMassFoodRequested() const;
	const uint32_t& getPercentStarved() const;
	uint32_t getDesireToEatSomethingAt(Block* block) const;
};
class EatEvent : public ScheduledEvent
{
	EatObjective& m_eatObjective;
public:
	EatEvent(step, EatObjective& eo);
	void execute();
};
class EatThreadedTask : ThreadedTask
{
	EatObjective& m_eatObjective;
	Block* m_blockResult;
	Actor* m_huntResult;
public:
	EatThreadedTask(EatObjective& eo);
	void readStep();
	void writeStep();
};
class EatObjective : public Objective
{
public:
	Actor& m_actor;
	ThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	Block* m_foodLocation;
	Item* m_foodItem;
	EatObjective(Actor& a);
	void execute();
	bool canEatAt(Block& block);
};
