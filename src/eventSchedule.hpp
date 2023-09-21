#pragma once
#include "eventSchedule.h"
#include "util.h"
template<class EventType>
class HasScheduledEvent
{
protected:
	ScheduledEventWithPercent* m_event;
	EventSchedule& m_schedule;
public:
	HasScheduledEvent(EventSchedule& s) : m_event(nullptr), m_schedule(s) {}
	template<typename ...Args>
	void schedule(Args&& ...args)
	{
		assert(m_event == nullptr);
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<EventType>(args...);
		m_event = event.get();
		m_schedule.schedule(std::move(event));
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
	void clearPointer()
	{
		assert(m_event != nullptr);
		m_event = nullptr;
	}
	[[nodiscard]]Percent percentComplete() const
	{
		assert(m_event != nullptr);
		return m_event->percentComplete();
	}
	[[nodiscard]]bool exists() const { return m_event != nullptr; }
	[[nodiscard]]const Step& getStep() const
	{
		assert(m_event != nullptr);
		return m_event->m_step;
	}
	[[nodiscard]]Step remainingSteps() const { return m_event->remaningSteps(); }
	[[nodiscard]]Step duration() const { return m_event->duration(); }
	~HasScheduledEvent() { maybeUnschedule(); }
};
template<class EventType>
class HasScheduledEventPausable : public HasScheduledEvent<EventType>
{
	Percent m_percent;
public:
	HasScheduledEventPausable(EventSchedule& es) : HasScheduledEvent<EventType>(es), m_percent(0) { }
	void pause()
	{
		m_percent = percentComplete();
		HasScheduledEvent<EventType>::unschedule();
	}
	void reset()
	{
		assert(HasScheduledEvent<EventType>::m_event == nullptr);
		m_percent = 0;
	}
	template<typename ...Args>
	void schedule(Step delay, Args&& ...args)
	{
		delay -= util::scaleByPercent(delay, m_percent);
		HasScheduledEvent<EventType>::schedule(delay, args...);
	}
	[[nodiscard]]Percent percentComplete() const
	{
		Percent output = m_percent;
		auto* event = HasScheduledEvent<EventType>::m_event;
		if(event != nullptr)
		{
			if(m_percent != 0)
				output += (event->percentComplete() * (100u - m_percent)) / 100u;
			else
				output = event->percentComplete();
		}
		return output;
	}
};
