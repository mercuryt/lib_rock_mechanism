#pragma once
#include "eventSchedule.h"
#include "util.h"
template<class EventType>
class HasScheduledEvent
{
protected:
	ScheduledEventWithPercent* m_event;
public:
	HasScheduledEvent() : m_event(nullptr) {}
	template<typename ...Args>
	void schedule(Args&& ...args)
	{
		assert(m_event == nullptr);
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<EventType>(args...);
		m_event = event.get();
		eventSchedule::schedule(std::move(event));
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
	uint32_t percentComplete() const
	{
		assert(m_event != nullptr);
		return m_event->percentComplete();
	}
	bool exists() const { return m_event != nullptr; }
	const uint32_t& getStep() const
	{
		assert(m_event != nullptr);
		return m_event->m_step;
	}
	~HasScheduledEvent() { maybeUnschedule(); }
};
template<class EventType>
class HasScheduledEventPausable : public HasScheduledEvent<EventType>
{
	uint32_t m_percent;
	public:
	HasScheduledEventPausable() : m_percent(0) { }
	uint32_t percentComplete() const
	{
		uint32_t output = m_percent;
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
	void schedule(uint32_t delay, Args&& ...args)
	{
		delay -= util::scaleByPercent(delay, m_percent);
		HasScheduledEvent<EventType>::schedule(delay, args...);
	}
};
