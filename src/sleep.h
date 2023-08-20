#pragma once

#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "config.h"
#include "eventSchedule.hpp"
#include "pathToBlockBaseThreadedTask.h"

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
	SleepEvent(const Step delay, MustSleep& ns);
	void execute();
	void clearReferences();
};
class TiredEvent final : public ScheduledEventWithPercent
{
	MustSleep& m_needsSleep;
public:
	TiredEvent(const Step delay, MustSleep& ns);
	void execute();
	void clearReferences();
};
// Find a place to sleep.
class SleepThreadedTask final : public PathToBlockBaseThreadedTask
{
	SleepObjective& m_sleepObjective;
public:
	SleepThreadedTask(SleepObjective& so);
	void readStep();
	void writeStep();
	void clearReferences();
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
	bool containsUnassigned(const Block& block) const { return m_unassigned.contains(const_cast<Block*>(&block)); }
};
