/*
 * Something which can be scheduled to happen on a future step.
 * Classes derived from ScheduledEvent are expected to provide a destructor which calls HasScheduledEvent::clearPointer.
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
	ScheduledEvent(uint32_t d);
	void cancel();
	virtual ~ScheduledEvent() {}
	virtual void execute() = 0;
	uint32_t remaningSteps() const { return m_step - ::s_step; }
};
class ScheduledEventWithPercent : public ScheduledEvent
{
public:
	uint32_t m_startStep;
	ScheduledEventWithPercent(uint32_t d) : ScheduledEvent(d), m_startStep(::s_step) {}
	uint32_t percentComplete() const
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
	void clear()
	{
		m_data.clear();
	}
};

inline ScheduledEvent::ScheduledEvent(uint32_t d) : m_step(d + ::s_step) {}
inline void ScheduledEvent::cancel() { m_eventSchedule->unschedule(this); }

class HasScheduledEvent
{
	ScheduledEventWithPercent* m_event;
	EventSchedule* m_eventSchedule;
public:
	HasScheduledEvent(EventSchedule& es) : m_event(nullptr), m_eventSchedule(&es) {}
	template<typename ...Args>
	void schedule(Args ...args)
	{
		assert(m_event == nullptr);
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<EventType>(args...);
		m_event = event.get();
		m_eventSchedule->schedule(event);
	}
	void unschedule()
	{
		assert(m_event != nullptr);
		m_event->cancel();
		m_event = nullptr;
	}
	void maybeUnschedule()
	{
		if(m_event != nullptr)
			unschedule();
	}
	uint32_t percentComplete() const
	{
		assert(m_event != nullptr);
		return m_event->percentComplete();
	}
	void setEventSchedule(EventSchedule& eventSchedule)
	{
		assert(m_event == nullptr);
		m_eventSchedule = eventSchedule;
	}
	void exists()
	{
		return m_event != nullptr;
	}
	void clearPointer()
	{
		m_event = nullptr;
	}
	~HasScheduledEvent()
	{
		if(m_event != nullptr)
			m_event.cancel();
	}
};
class HasScheduledEventPausable : HasScheduledEvent<EventType>
{
	uint32_t m_percent;
	HasScheduledEventPausable(EventSchedule& es) : HasScheduledEvent<EventType>(es), m_percent(0)
public:
	uint32_t percentComplete() const
	{
		uint32_t output = m_percent;
		if(!HasScheduledEvent<EventType>::m_event.empty())
		{
			if(m_percent != 0)
				output += (m_event->percentComplete() * (100u - m_percent)) / 100u;
			else
				output = m_event->percentComplete();
		}
		return output;
	}
	void pause()
	{
		m_percent = percentComplete();
		unschedule();
	}
	void reset()
	{
		assert(m_event == nullptr);
		m_percent = 0;
	}
	template<typename ...Args>
	void schedule(uint32_t step, Args ...args)
	{
		uint32_t delay = step - ::step;
		step -= util::scaleByPercent(delay, m_percent);
		HasScheduledEvent::schedule(step, args...);
	}
};
