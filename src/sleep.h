#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"

template<class NeedsSleep>
class SleepEvent : ScheduledEventWithPercent
{
	NeedsSleep& m_needsSleep;
	SleepEvent(uint32_t step, NeedsSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
	void execute(){ m_needsSleep.callback(); }
	~SleepEvent(){ m_needsSleep.m_sleepEvent.clearPointer(); }
}
template<class SleepObjective, class Block>
class SleepThreadedTask : ThreadedTask
{
	SleepObjective& m_sleepObjective;
	std::vector<Block*> m_result;
	SleepThreadedTask(SleepObjective& so) : m_sleepObjective(so) { }
	void readStep()
	{
		auto& actor = m_sleepObjective.m_needsSleep.m_actor;
		auto condition = [&](Block* block)
		{
			return m_sleepObjective.canSleepAt(*block);
		}
		m_result = path::getForActorToPredicate(actor, condition);
	}
	void writeStep()
	{
		auto& actor = m_sleepObjective.m_needsSleep.m_actor;
		if(m_result.empty())
			actor.m_player.m_eventPublisher.publish(PublishedEventType::CannotFindPlaceToSleep);
		else
		{
			// Try again if now no good.
			if(!m_sleepObjective.canSleepAt(m_result.back()))
				m_sleepObjective.m_threadedTask.create(m_sleepObjective);
			else
			{
				actor.setPath(m_result);
				m_sleepObjective.m_needsSleep.m_location = m_result.back();
			}
		}
	}
}
template<class NeedsSleep, class Block>
class SleepObjective : Obective
{
	NeedsSleep& m_needsSleep;
	HasThreadedTask<SleepThreadedTask<SleepObjective, Block>> m_threadedTask;
	SleepObjective(NeedsSleep& ns) : Objective(Config::sleepObjectivePriority), m_needsSleep(ns) { }
	void execute()
	{
		if(m_needsSleep.m_location == nullptr)
		{
			m_threadedTask.create(*this);
		}
		else
		{
			if(canSleepAt(m_needsSleep.m_actor.m_location))
				m_needsSleep.sleep();
			else
				m_needsSleep.m_actor.setDestination(*m_location);
		}
	}
	bool canSleepAt(Block& block)
	{
			return !block->m_reservable.isFullyReserved() && block->m_temperature.isSafeFor(m_needsSleep.actor);
	}
}
template<class Actor, class Block>
class NeedsSleep
{
	Actor& m_actor;
	Block& m_location;
	HasScheduledEvent<SleepEvent> m_sleepEvent;
	uint32_t m_percentSleepCompleted;
	bool m_needsSleep;
	bool m_isAwake;
	NeedsSleep(Actor& a) : m_actor(a), m_needsSleep(false), m_isAwake(true)
	{
		m_sleepEvent.schedule(::s_step + m_actor.m_animalType.stepsNeedsSleepFrequency, *this);
	}
	void callback()
	{
		if(m_isAwake == false)
		{
			m_isAwake = true;
			m_needsSleep = false;
			m_sleepEvent.schedule(::s_step + m_actor.m_animalType.stepsNeedsSleepFrequency, *this);
		}
		else if(m_needsSleep)
			sleep();
		else
		{
			m_needsSleep = true;
			m_sleepEvent.schedule(::s_step + m_actor.m_animalType.stepsTillSleepOveride, *this);
			m_actor.m_hasObjectives.addNeed(std::make_unique<SleepObjective>(*this));
		}
	}
	void sleep()
	{
		m_isAwake = false;
		m_sleepEvent.unschedule();
		m_sleepEvent.schedule(::s_step + m_actor.m_animal.stepsSleepDuration, *this);
	}
}
