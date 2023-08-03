#pragma once

#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "config.h"
#include "eventSchedule.hpp"

#include <vector>

class Area;
class Actor;
class Block;
class Item;
class SleepEvent;
class TiredEvent;
class SleepObjective;
class MustSleep final 
{
	Actor& m_actor;
	Block* m_location;
	HasScheduledEventPausable<SleepEvent> m_sleepEvent;
	HasScheduledEvent<TiredEvent> m_tiredEvent;
	SleepObjective* m_objective;
	bool m_needsSleep;
	bool m_isAwake;
public:
	MustSleep(Actor& a);
	void tired();
	void sleep();
	void wakeUp();
	void makeSleepObjective();
	void wakeUpEarly();
	friend class SleepEvent;
	friend class TiredEvent;
	friend class SleepThreadedTask;
	friend class SleepObjective;
};
class SleepEvent final : public ScheduledEventWithPercent
{
	MustSleep& m_needsSleep;
public:
	SleepEvent(uint32_t step, MustSleep& ns);
	void execute();
	~SleepEvent();
};
class TiredEvent final : public ScheduledEventWithPercent
{
	MustSleep& m_needsSleep;
public:
	TiredEvent(uint32_t step, MustSleep& ns);
	void execute();
	~TiredEvent();
};
// Find a place to sleep.
class SleepThreadedTask final : public ThreadedTask
{
	SleepObjective& m_sleepObjective;
	std::vector<Block*> m_result;
public:
	SleepThreadedTask(SleepObjective& so);
	void readStep();
	void writeStep();
};
class SleepObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<SleepThreadedTask> m_threadedTask;
public:
	SleepObjective(Actor& a) : Objective(Config::sleepObjectivePriority), m_actor(a) { }
	void execute();
	void cancel() {}
	uint32_t desireToSleepAt(Block& block);
	~SleepObjective();
	friend class SleepThreadedTask;
};
class HasSleepingSpots final
{
	std::unordered_set<Block*> m_unassigned;
public:
	bool containsUnassigned(Block& block) const { return m_unassigned.contains(&block); }
};
