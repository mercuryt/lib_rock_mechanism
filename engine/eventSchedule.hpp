#pragma once
#include "eventSchedule.h"
#include "numericTypes/types.h"
#include "util.h"
#include "numericTypes/index.h"
#include "dataStructures/strongVector.h"
class Simulation;
template<class EventType>
class HasScheduledEvent
{
protected:
	// m_schedule is pointer rather then a reference so it can be moved.
	EventSchedule* m_schedule = nullptr;
	ScheduledEvent* m_event = nullptr;
public:
	HasScheduledEvent(EventSchedule& s) : m_schedule(&s)
	{
		assert(&s != nullptr);
	}
	template<typename ...Args>
	void schedule(Args&& ...args)
	{
		assert(!exists());
		std::unique_ptr<ScheduledEvent> event = std::make_unique<EventType>(args...);
		m_event = event.get();
		m_schedule->schedule(std::move(event));
	}
	void unschedule()
	{
		assert(exists());
		m_event->cancel(m_schedule->getSimulation(), m_schedule->getArea());
		m_event = nullptr;
	}
	void maybeUnschedule()
	{
		if(exists())
			unschedule();
	}
	void clearPointer() { assert(exists()); m_event = nullptr; }
	[[nodiscard]] Percent percentComplete() const { assert(exists()); return m_event->percentComplete(m_schedule->getSimulation()); }
	[[nodiscard]] float fractionComplete() const { assert(exists()); return m_event->fractionComplete(m_schedule->getSimulation()); }
	[[nodiscard]] bool exists() const { return m_event != nullptr; }
	[[nodiscard]] const Step& getStep() const { assert(exists()); return m_event->m_step; }
	[[nodiscard]] Step getStartStep() const { assert(exists()); return m_event->m_startStep; }
	[[nodiscard]] Step remainingSteps() const { assert(exists()); return m_event->remaningSteps(m_schedule->getSimulation()); }
	[[nodiscard]] Step elapsedSteps() const { assert(exists()); return m_event->elapsedSteps(m_schedule->getSimulation()); }
	[[nodiscard]] Step duration() const { assert(exists()); return m_event->duration(); }
	[[nodiscard]] ScheduledEvent* getEvent() { return m_event; }
	~HasScheduledEvent()
	{
		if(exists())
			m_event->cancel(m_schedule->getSimulation(), m_schedule->getArea());
	}
};
template<class EventType>
class HasScheduledEventPausable : public HasScheduledEvent<EventType>
{
	Step m_elapsedSteps;
public:
	HasScheduledEventPausable(EventSchedule& es) : HasScheduledEvent<EventType>(es), m_elapsedSteps(Step::create(0)) { }
	void pause()
	{
		assert(HasScheduledEvent<EventType>::exists());
		m_elapsedSteps += HasScheduledEvent<EventType>::elapsedSteps();
		HasScheduledEvent<EventType>::unschedule();
	}
	void reset()
	{
		assert(HasScheduledEvent<EventType>::m_event == nullptr);
		m_elapsedSteps = Step::create(0);
	}
	template<typename ...Args>
	void resume(Step delay, Args&& ...args)
	{
		Step adjustedDelay = m_elapsedSteps < delay ? delay - m_elapsedSteps : Step::create(1);
		HasScheduledEvent<EventType>::schedule(adjustedDelay, args...);
	}
	void setElapsedSteps(Step steps) { m_elapsedSteps = steps; }
	[[nodiscard]] Percent percentComplete() const
	{
		const ScheduledEvent* event = HasScheduledEvent<EventType>::m_event;
		assert(event);
		// Combine the steps from this event run with saved steps from previous ones.
		Step fullDuration = event->duration() + m_elapsedSteps;
		Simulation& simulation = HasScheduledEvent<EventType>::m_schedule->getSimulation();
		Step fullElapsed = event->elapsedSteps(simulation) + m_elapsedSteps;
		return util::fractionToPercent(fullElapsed.get(), fullDuration.get());
	}
	// Alias exits as isPaused.
	[[nodiscard]] bool isPaused() const { return !HasScheduledEvent<EventType>::exists(); }
	[[nodiscard]] Step getStoredElapsedSteps() const { return m_elapsedSteps; }
};
template<class EventType, class Index>
class HasScheduledEvents
{
protected:
	EventSchedule& m_schedule;
	StrongVector<EventType*, Index> m_events;
public:
	HasScheduledEvents(EventSchedule& s) : m_schedule(s) { assert(&s); }
	void load(Simulation& simulation, const Json& data, const Index& size)
	{
		m_events.resize(size);
		for(auto iter = data.begin(); iter != data.end(); ++iter)
		{
			const Index& index = Index::create(std::stoi(iter.key()));
			auto event = std::make_unique<EventType>(simulation, iter.value());
			m_events[index] = event.get();
			m_schedule.schedule(std::move(event));
		}
	}
	void resize(const Index& size)
	{
		m_events.resize(size);
	}
	template<typename ...Args>
	void schedule(const Index& index, Args&& ...args)
	{
		assert(m_events.size() > index.get());
		assert(m_events[index] == nullptr);
		std::unique_ptr<ScheduledEvent> event = std::make_unique<EventType>(args...);
		m_events[index] = static_cast<EventType*>(event.get());
		m_schedule.schedule(std::move(event));
	}
	void unschedule(const Index& index)
	{
		assert(m_events[index] != nullptr);
		m_events[index]->cancel(m_schedule.getSimulation(), m_schedule.getArea());
		m_events[index] = nullptr;
	}
	void maybeUnschedule(const Index& index)
	{
		if(m_events[index] != nullptr)
			unschedule(index);
	}
	void clearPointer(const Index& index)
	{
		assert(m_events[index] != nullptr);
		m_events[index] = nullptr;
	}
	void moveIndex(const Index& oldIndex, const Index& newIndex)
	{
		m_events[newIndex] = m_events[oldIndex];
		m_events[newIndex]->onMoveIndex(oldIndex, newIndex);
	}
	void sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<uint32_t, Index>> sortOrder)
	{
		m_events.sortRangeWithOrder(begin, end, sortOrder);
	}
	[[nodiscard]] bool contains(const Index& index) { return m_events[index] != nullptr; }
	[[nodiscard]] auto at(const Index& index) -> EventType& { return *m_events[index]; }
	[[nodiscard]] auto operator[](const Index& index) -> EventType& { return at(index); }
	[[nodiscard]] Percent percentComplete(const Index& index) const
	{
		assert(m_events[index] != nullptr);
		return m_events[index]->percentComplete(m_schedule.getSimulation());
	}
	[[nodiscard]] float fractionComplete(const Index& index) const
	{
		assert(m_events[index] != nullptr);
		return m_events[index]->fractionComplete(m_schedule.getSimulation());
	}
	[[nodiscard]] bool exists(const Index& index) const { return m_events[index] != nullptr; }
	[[nodiscard]] const Step& getStep(const Index& index) const
	{
		assert(m_events[index] != nullptr);
		return m_events[index]->m_step;
	}
	[[nodiscard]] Step getStartStep(const Index& index) const { return m_events[index]->m_startStep; }
	[[nodiscard]] Step remainingSteps(const Index& index) const { return m_events[index]->remaningSteps(m_schedule.getSimulation()); }
	[[nodiscard]] Step elapsedSteps(const Index& index) const { return m_events[index]->elapsedSteps(m_schedule.getSimulation()); }
	[[nodiscard]] Step duration(const Index& index) const { return m_events[index]->duration(); }
	[[nodiscard]] ScheduledEvent* getEvent(const Index& index) { return m_events[index]; }
	[[nodiscard]] Json toJson() const
	{
		Json output = Json::object();
		int i = 0;
		for(const EventType* event : m_events)
		{
			if(event != nullptr)
				output[std::to_string(i)] = event->toJson();
			i++;
		}
		return output;
	}
	~HasScheduledEvents()
	{
		for(EventType* event : m_events)
			if(event != nullptr)
				event->cancel(m_schedule.getSimulation(), m_schedule.getArea());
	}
};
template<class EventType>
void to_json(Json& data, const HasScheduledEvent<EventType>& hasScheduledEvent) { data = hasScheduledEvent.toJson(); }
template<class EventType, class Index>
void to_json(Json& data, const HasScheduledEvents<EventType, Index>& hasScheduledEvents) { data = hasScheduledEvents.toJson(); }
