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
	virtual ~ScheduledEvent() {}
	virtual void execute() = 0;
};

class HasScheduledEvents
{
public:
	std::unordered_map<uint32_t, std::list<ScheduledEvent*>> m_scheduledEvents;
	void scheduleEvent(ScheduledEvent* scheduledEvent)
	{
		m_scheduledEvents[scheduledEvent->m_step].push_back(scheduledEvent);
	}
	void unscheduleEvent(ScheduledEvent* scheduledEvent)
	{
		assert(m_scheduledEvents.contains(scheduledEvent->m_step));
		m_scheduledEvents[scheduledEvent->m_step].remove(scheduledEvent);
		delete scheduledEvent;
	}
	void executeScheduledEvents(uint32_t stepNumber)
	{
		if(!m_scheduledEvents.contains(stepNumber))
			return;
		for(ScheduledEvent* scheduledEvent : m_scheduledEvents[stepNumber])
		{
			scheduledEvent->execute();
			// note: scheduled events are responsible for cleaning up their own references when deleted.
			delete scheduledEvent;
		}
		m_scheduledEvents.erase(stepNumber);
	}
	void resetScheduledEvents()
	{
		for(auto pair : m_scheduledEvents)
			for(ScheduledEvent* scheduledEvent : pair.second)
				delete scheduledEvent;
		m_scheduledEvents.clear();
	}
	~HasScheduledEvents()
	{
		resetScheduledEvents();
	}
};
