#pragma once
#include "eventSchedule.h"
#include "util.h"
template<class EventType>
class HasScheduledEvent
{
protected:
	EventSchedule& m_schedule;
	ScheduledEvent* m_event = nullptr;
public:
	HasScheduledEvent(EventSchedule& s) : m_schedule(s) 
	{ 
		assert(&s);
	}
	template<typename ...Args>
	void schedule(Args&& ...args)
	{
		assert(m_event == nullptr);
		std::unique_ptr<ScheduledEvent> event = std::make_unique<EventType>(args...);
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
	[[nodiscard]] Percent percentComplete() const
	{
		assert(m_event != nullptr);
		return m_event->percentComplete();
	}
	[[nodiscard]] float fractionComplete() const
	{
		assert(m_event != nullptr);
		return m_event->fractionComplete();
	}
	[[nodiscard]] bool exists() const { return m_event != nullptr; }
	[[nodiscard]] const Step& getStep() const
	{
		assert(m_event != nullptr);
		return m_event->m_step;
	}
	[[nodiscard]] Step getStartStep() const { return m_event->m_startStep; }
	[[nodiscard]] Step remainingSteps() const { return m_event->remaningSteps(); }
	[[nodiscard]] Step elapsedSteps() const { return m_event->elapsedSteps(); }
	[[nodiscard]] Step duration() const { return m_event->duration(); }
	[[nodiscard]] ScheduledEvent* getEvent() { return m_event; }
	~HasScheduledEvent() 
	{ 
		if(m_event != nullptr)
			m_event->cancel();
	}
};
template<class EventType>
class HasScheduledEventPausable : public HasScheduledEvent<EventType>
{
	Step m_elapsedSteps;
public:
	HasScheduledEventPausable(EventSchedule& es) : HasScheduledEvent<EventType>(es), m_elapsedSteps(0) { }
	void pause()
	{
		assert(HasScheduledEvent<EventType>::exists());
		m_elapsedSteps += HasScheduledEvent<EventType>::elapsedSteps();
		HasScheduledEvent<EventType>::unschedule();
	}
	void reset()
	{
		assert(HasScheduledEvent<EventType>::m_event == nullptr);
		m_elapsedSteps = 0;
	}
	template<typename ...Args>
	void resume(Step delay, Args&& ...args)
	{
		Step adjustedDelay = m_elapsedSteps < delay ? delay - m_elapsedSteps : 1;
		HasScheduledEvent<EventType>::schedule(adjustedDelay, args...);
	}
	void setElapsedSteps(Step steps) { m_elapsedSteps = steps; }
	[[nodiscard]] Percent percentComplete() const
	{
		const ScheduledEvent* event = HasScheduledEvent<EventType>::m_event;
		assert(event);
		// Combine the steps from this event run with saved steps from previous ones.
		Step fullDuration = event->duration() + m_elapsedSteps;
		Step fullElapsed = event->elapsedSteps() + m_elapsedSteps;
		return util::fractionToPercent(fullElapsed, fullDuration);
	}
	// Alias exits as isPaused.
	[[nodiscard]] bool isPaused() const { return !HasScheduledEvent<EventType>::exists(); }
	[[nodiscard]] Step getStoredElapsedSteps() const { return m_elapsedSteps; }
};
