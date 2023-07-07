#pragma once

#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "actor.h"
#include "block.h"

#include <vector>

class SleepEvent;
class TiredEvent;
class SleepObjective;
class NeedsSleep final 
{
	Actor& m_actor;
	Block& m_location;
	HasScheduledEventPausable<SleepEvent> m_sleepEvent;
	HasScheduledEvent<TiredEvent> m_tiredEvent;
	SleepObjective* m_objective;
	bool m_needsSleep;
	bool m_isAwake;
public:
	NeedsSleep(Actor& a);
	void tired();
	void sleep();
	void wakeUp();
	void makeSleepObjective();
	void wakeUpEarly();
};
class SleepEvent final : public ScheduledEventWithPercent
{
	NeedsSleep& m_needsSleep;
public:
	SleepEvent(uint32_t step, NeedsSleep& ns);
	void execute();
	~SleepEvent();
};
class TiredEvent final : public ScheduledEventWithPercent
{
	NeedsSleep& m_needsSleep;
public:
	TiredEvent(uint32_t step, NeedsSleep& ns);
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
	NeedsSleep& m_needsSleep;
	HasThreadedTask<SleepThreadedTask> m_threadedTask;
public:
	SleepObjective(NeedsSleep& ns);
	void execute();
	uint32_t desireToSleepAt(Block& block);
	~SleepObjective();
};
