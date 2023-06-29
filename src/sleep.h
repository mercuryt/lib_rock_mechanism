#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"

class SleepEvent : ScheduledEventWithPercent
{
	NeedsSleep& m_needsSleep;
	SleepEvent(uint32_t step, NeedsSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
	void execute(){ m_needsSleep.wakeUp(); }
	~SleepEvent(){ m_needsSleep.m_sleepEvent.clearPointer(); }
};
class TiredEvent : ScheduledEventWithPercent
{
	NeedsSleep& m_needsSleep;
	TiredEvent(uint32_t step, NeedsSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
	void execute(){ m_needsSleep.tired(); }
	~SleepEvent(){ m_needsSleep.m_tiredEvent.clearPointer(); }
};
// Find a place to sleep.
class SleepThreadedTask : ThreadedTask
{
	SleepObjective& m_sleepObjective;
	std::vector<Block*> m_result;
	SleepThreadedTask(SleepObjective& so) : m_sleepObjective(so) { }
	void readStep()
	{
		auto& actor = m_sleepObjective.m_needsSleep.m_actor;
		assert(m_sleepObjective.m_needsSleep.m_location == nullptr);
		Block* outdoorCandidate = nullptr;
		Block* indoorCandidate = nullptr;
		auto condition = [&](Block* block)
		{
			uint32_t desire = m_sleepObjective.desireToSleepAt(*block);
			if(desire == 3)
				return true;
			else if(indoorCandidate == nullptr && desire == 2)
				indoorCandidate = block;	
			else if(outdoorCandidate == nullptr && desire == 1)
				outdoorCandidate = block;	
			return false;
		};
		m_result = path::getForActorToPredicate(actor, condition);
		if(m_result.empty())
			if(indoorCandidate != nullptr)
				m_result = path::getForActorTo(indoorCandidate);
			else if(outdoorCandidate != nullptr)
				m_result = path::getForActorTo(outdoorCandidate);
	}
	void writeStep()
	{
		auto& actor = m_sleepObjective.m_needsSleep.m_actor;
		if(m_result.empty*()
			m_actor.m_hasObjectives.cannotCompleteNeed(m_sleepObjective);
		else
			m_actor.setPath(m_result);
	}
};
class SleepObjective : Obective
{
	NeedsSleep& m_needsSleep;
	HasThreadedTask<SleepThreadedTask<SleepObjective, Block>> m_threadedTask;
	SleepObjective(NeedsSleep& ns) : Objective(Config::sleepObjectivePriority), m_needsSleep(ns) { }
	void execute()
	{
		if(m_needsSleep.m_actor.m_location == m_needsSleep.m_location)
		{
			assert(m_needsSleep.m_location != nullptr);
			m_needsSleep.sleep();
		}
		else if(m_needsSleep.m_location == nullptr)
			m_threadedTask.create(*this);
		else
			m_needsSleep.m_actor.setDestination(*m_location);
	}
	uint32_t desireToSleepAt(Block& block)
	{
			if(block->m_reservable.isFullyReserved() || !block->m_temperature.isSafeFor(m_needsSleep.actor))
				return 0;
			if(block->m_outdoors)
				return 1;
			if(block->m_indoors)
				return 2;
			if(block->m_area->m_hasSleepingSpots.contains(block))
				return 3;
	}
	~SleepObjective() { m_needsSleep.m_objective = nullptr; }
};
class NeedsSleep
{
	Actor& m_actor;
	Block& m_location;
	HasScheduledEventPausable<SleepEvent> m_sleepEvent;
	HasScheduledEvent<TiredEvent> m_tiredEvent;
	SleepObjective* m_objective;
	bool m_needsSleep;
	bool m_isAwake;
	NeedsSleep(Actor& a) : m_actor(a), m_needsSleep(false), m_isAwake(true)
	{
		m_tiredEvent.schedule(::s_step + m_actor.getStepsNeedsSleepFrequency(), *this);
	}
	void tired()
	{
		assert(m_isAwake);
		if(m_needsSleep)
			sleep();
		else
		{
			m_needsSleep = true;
			m_tiredEvent.schedule(::s_step + m_actor.getStepsTillSleepOveride(), *this);
			makeSleepObjective();
		}
	}
	void sleep()
	{
		assert(m_isAwake);
		m_isAwake = false;
		m_tiredEvent.unschedule();
		m_sleepEvent.schedule(::s_step + m_actor.getStepsSleepDuration(), *this);
	}
	void wakeUp()
	{
		assert(!m_isAwake);
		m_isAwake = true;
		m_tiredEvent.schedule(::s_step + m_actor.getStepsNeedsSleepFrequency(), *this);
	}
	void makeSleepObjective()
	{
		assert(m_isAwake);
		assert(m_objective == nullptr);
		std::unique_ptr<Objective> objective = std::make_unique<SleepObjective>(*this);
		m_objective = objective.get();
		m_actor.m_hasObjectives.addNeed(objective);
	}
	void wakeUpEarly()
	{
		assert(!m_isAwake);
		assert(m_needsSleep == true);
		m_isAwake = true;
		m_sleepEvent.pause();
	}
};
