/*
 * Something which can be scheduled to happen on a future step.
 */

#pragma once
#include <list>
#include <unordered_map>
#include <memory>

class EventSchedule;
class ScheduledEvent
{
public:
	uint32_t m_step;
	EventSchedule* m_eventSchedule;
	ScheduledEvent(uint32_t s);
	void cancel();
	virtual ~ScheduledEvent() {}
	virtual void execute() = 0;
};

class EventSchedule
{
public:
	std::unordered_map<uint32_t, std::list<std::unique_ptr<ScheduledEvent>>> m_data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent)
	{
		scheduledEvent->m_eventSchedule = this;
		m_data[scheduledEvent->m_step].push_back(std::move(scheduledEvent));
	}
	void unschedule(const ScheduledEvent* scheduledEvent)
	{
		assert(m_data.contains(scheduledEvent->m_step));
		m_data.at(scheduledEvent->m_step).remove_if([&](auto& eventPtr){ return &*eventPtr == scheduledEvent; });
	}
	void execute(uint32_t stepNumber)
	{
		auto found = m_data.find(stepNumber);
		if(found == m_data.end())
			return;
		for(std::unique_ptr<ScheduledEvent>& scheduledEvent : found->second)
			scheduledEvent->execute();
		m_data.erase(stepNumber);
	}
};

ScheduledEvent::ScheduledEvent(uint32_t s) : m_step(s) {}
void ScheduledEvent::cancel() { m_eventSchedule->unschedule(this); }