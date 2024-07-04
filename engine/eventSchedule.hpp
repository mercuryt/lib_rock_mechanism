#pragma once
#include "eventSchedule.h"
#include "types.h"
#include "util.h"
class Simulation;
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
		m_event->cancel(m_schedule.getSimulation(), m_schedule.getArea());
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
		return m_event->percentComplete(m_schedule.getSimulation());
	}
	[[nodiscard]] float fractionComplete() const
	{
		assert(m_event != nullptr);
		return m_event->fractionComplete(m_schedule.getSimulation());
	}
	[[nodiscard]] bool exists() const { return m_event != nullptr; }
	[[nodiscard]] const Step& getStep() const
	{
		assert(m_event != nullptr);
		return m_event->m_step;
	}
	[[nodiscard]] Step getStartStep() const { return m_event->m_startStep; }
	[[nodiscard]] Step remainingSteps() const { return m_event->remaningSteps(m_schedule.getSimulation()); }
	[[nodiscard]] Step elapsedSteps() const { return m_event->elapsedSteps(m_schedule.getSimulation()); }
	[[nodiscard]] Step duration() const { return m_event->duration(); }
	[[nodiscard]] ScheduledEvent* getEvent() { return m_event; }
	~HasScheduledEvent() 
	{ 
		if(m_event != nullptr)
			m_event->cancel(m_schedule.getSimulation(), m_schedule.getArea());
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
		Simulation& simulation = HasScheduledEvent<EventType>::m_schedule.getSimulation();
		Step fullElapsed = event->elapsedSteps(simulation) + m_elapsedSteps;
		return util::fractionToPercent(fullElapsed, fullDuration);
	}
	// Alias exits as isPaused.
	[[nodiscard]] bool isPaused() const { return !HasScheduledEvent<EventType>::exists(); }
	[[nodiscard]] Step getStoredElapsedSteps() const { return m_elapsedSteps; }
};
template<class EventType>
class HasScheduledEvents
{
protected:
	EventSchedule& m_schedule;
	std::vector<ScheduledEvent*> m_events;
public:
	HasScheduledEvents(EventSchedule& s) : m_schedule(s) { assert(&s); }
	void resize(HasShapeIndex size)
	{
		m_events.resize(size);
	}
	template<typename ...Args>
	void schedule(HasShapeIndex index, Args&& ...args)
	{
		assert(m_events.size() > index);
		assert(m_events.at(index) == nullptr);
		std::unique_ptr<ScheduledEvent> event = std::make_unique<EventType>(args...);
		m_events.at(index) = event.get();
		m_schedule.schedule(std::move(event));
	}
	void unschedule(HasShapeIndex index)
	{
		assert(m_events.at(index) != nullptr);
		m_events.at(index)->cancel(m_schedule.getSimulation(), m_schedule.getArea());
		m_events.at(index) = nullptr;
	}
	void maybeUnschedule(HasShapeIndex index)
	{
		if(m_events.at(index) != nullptr)
			unschedule(index);
	}
	void clearPointer(HasShapeIndex index)
	{
		assert(m_events.at(index) != nullptr);
		m_events.at(index) = nullptr;
	}
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
	{
		std::swap(m_events[oldIndex], m_events[newIndex]);
	}
	[[nodiscard]] ScheduledEvent* at(HasShapeIndex index) { return m_events.at(index); }
	[[nodiscard]] ScheduledEvent* operator[](HasShapeIndex index) { return at(index); }
	[[nodiscard]] Percent percentComplete(HasShapeIndex index) const
	{
		assert(m_events.at(index) != nullptr);
		return m_events.at(index)->percentComplete(m_schedule.getSimulation());
	}
	[[nodiscard]] float fractionComplete(HasShapeIndex index) const
	{
		assert(m_events.at(index) != nullptr);
		return m_events.at(index)->fractionComplete(m_schedule.getSimulation());
	}
	[[nodiscard]] bool exists(HasShapeIndex index) const { return m_events.at(index) != nullptr; }
	[[nodiscard]] const Step& getStep(HasShapeIndex index) const
	{
		assert(m_events.at(index) != nullptr);
		return m_events.at(index)->m_step;
	}
	[[nodiscard]] Step getStartStep(HasShapeIndex index) const { return m_events.at(index)->m_startStep; }
	[[nodiscard]] Step remainingSteps(HasShapeIndex index) const { return m_events.at(index)->remaningSteps(m_schedule.getSimulation()); }
	[[nodiscard]] Step elapsedSteps(HasShapeIndex index) const { return m_events.at(index)->elapsedSteps(m_schedule.getSimulation()); }
	[[nodiscard]] Step duration(HasShapeIndex index) const { return m_events.at(index)->duration(); }
	[[nodiscard]] ScheduledEvent* getEvent(HasShapeIndex index) { return m_events.at(index); }
	~HasScheduledEvents() 
	{ 
		for(ScheduledEvent* event : m_events)
			if(event != nullptr)
				event->cancel(m_schedule.getSimulation(), m_schedule.getArea());
	}
};
