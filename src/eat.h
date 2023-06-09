#pragma once
#include "util.h"
#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"

class Item;
class Actor;
class EatObjective;
class HungerEvent;

class MustEat
{
	Actor& m_actor;
	uint32_t m_massFoodRequested;
	HasScheduledEvent<HungerEvent> m_hungerEvent;
public:
	MustEat(Actor& a);
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
	EatEvent(uint32_t delay, EatObjective& eo);
	void execute();
};
class HungerEvent : public ScheduledEvent
{
	EatObjective& m_eatObjective;
public:
	HungerEvent(uint32_t delay, EatObjective& eo);
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
	HasThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	Block* m_foodLocation;
	Item* m_foodItem;
	EatObjective(Actor& a);
	void execute();
	bool canEatAt(Block& block);
};
