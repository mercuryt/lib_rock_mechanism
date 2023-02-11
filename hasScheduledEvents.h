/*
 * Something which can be scheduled to happen on a future step.
 */

#pragma once
#include <list>
#include <unordered_map>

class ScheduledEvent
{
public:
	uint32_t m_step;
	ScheduledEvent(uint32_t s) : m_step(s) {}
	virtual void execute();
	friend class HasScheduledEvents;
};

class HasScheduledEvents
{
	std::unordered_map<uint32_t, std::list<ScheduledEvent*>> m_scheduledEvents;
public:
	void scheduleEvent(ScheduledEvent* scheduledEvent)
	{
		m_scheduledEvents[scheduledEvent->m_step].push_back(scheduledEvent);
	}
	void unscheduleEvent(ScheduledEvent* scheduledEvent)
	{

		m_scheduledEvents[scheduledEvent->m_step].remove(scheduledEvent);
		delete scheduledEvent;
	}
	void exeuteScheduledEvents(uint32_t stepNumber)
	{
		for(ScheduledEvent* scheduledEvent : m_scheduledEvents[stepNumber])
		{
			scheduledEvent->execute();
			// note: scheduled events are responsible for cleaning up their own references when deleted.
			delete scheduledEvent;
		}
		m_scheduledEvents.erase(stepNumber);
	}
};
/*
 *  Try to enter next step on route. 
 */
class MoveEvent : public ScheduledEvent
{
public:
	Actor* m_actor;
	MoveEvent(uint32_t s, Actor* a);
	void execute();
	~MoveEvent();
};
