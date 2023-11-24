#pragma once

#include "objective.h"
#include "config.h"
#include "findsPath.h"
#include "threadedTask.hpp"
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
	void passout(Step duration);
	void sleep(Step duration, bool force = false);
	void wakeUp();
	void makeSleepObjective();
	void wakeUpEarly();
	void setLocation(Block& block);
	void onDeath();
	void notTired();
	bool isAwake() const { return m_isAwake; }
	bool getNeedsSleep() const { return m_needsSleep; }
	Block* getLocation() { return m_location; }
	friend class SleepEvent;
	friend class TiredEvent;
	friend class SleepThreadedTask;
	friend class SleepObjective;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasTiredEvent() const { return m_tiredEvent.exists(); }
	[[maybe_unused, nodiscard]] SleepObjective* getObjective() { return m_objective; }
};
class SleepEvent final : public ScheduledEventWithPercent
{
	MustSleep& m_needsSleep;
	bool m_force;
public:
	SleepEvent(const Step delay, MustSleep& ns, bool force);
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
class SleepThreadedTask final : public ThreadedTask
{
	SleepObjective& m_sleepObjective;
	FindsPath m_findsPath;
	bool m_sleepAtCurrentLocation;
	bool m_noWhereToSleepFound;
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
	bool m_noWhereToSleepFound;
public:
	SleepObjective(Actor& a);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	bool onNoPath();
	[[nodiscard]] uint32_t desireToSleepAt(const Block& block) const;
	[[nodiscard]] std::string name() const { return "sleep"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Sleep; }
	[[nodiscard]] bool isNeed() const { return true; }
	~SleepObjective();
	friend class SleepThreadedTask;
	friend class MustSleep;
	// For testing.
	[[maybe_unused, nodiscard]] bool threadedTaskExists() const { return m_threadedTask.exists(); }
};
class HasSleepingSpots final
{
	std::unordered_set<Block*> m_unassigned;
public:
	bool containsUnassigned(const Block& block) const { return m_unassigned.contains(const_cast<Block*>(&block)); }
};
