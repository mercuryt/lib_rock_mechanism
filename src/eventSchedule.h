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
class ScheduledEventWithPercent: public ScheduledEvent
{
public:
	uint32_t m_startStep;
	ScheduledEventWithPercent(uint32_t s) : ScheduledEvent(s), m_startStep(s_step) {}
	uint32_t percentComplete()
	{
		float totalSteps = m_step - m_startStep;
		float elapsedSteps = s_step - m_startStep;
		return (elapsedSteps / totalSteps) * 100;
	}
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
	//TODO: why doesn't this work?
	/*
	template<class EventType, class ...Types>
	void schedule(Types&... args)
	{
		schedule(std::move(std::make_unique<EventType>(args...)));
	}
	template<class EventType, class ...Types>
	ScheduledEvent* emplaceSchedule(Types&... args)
	{
		auto event = std::make_unique<EventType>(args...);
		ScheduledEvent* output = event.get();
		schedule(std::move(event));
		return output;
	}
	*/
	void unschedule(const ScheduledEvent* scheduledEvent)
	{
		// Unscheduling an event which is scheduled for the current step could mean modifing the event schedule while it is being iterated. Don't allow.
		assert(scheduledEvent->m_step != s_step);
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

inline ScheduledEvent::ScheduledEvent(uint32_t s) : m_step(s) {}
inline void ScheduledEvent::cancel() { m_eventSchedule->unschedule(this); }
