#include "eventSchedule.h"
#include "simulation.h"
#include "util.h"
#include "area.h"
#include <cassert>
ScheduledEvent::ScheduledEvent(uint32_t d) : m_step(d + simulation::step){}
void ScheduledEvent::cancel() { eventSchedule::unschedule(*this); }
uint32_t ScheduledEvent::remaningSteps() const { return m_step - simulation::step; }

ScheduledEventWithPercent::ScheduledEventWithPercent(uint32_t d) : ScheduledEvent(d), m_startStep(simulation::step) {}
uint32_t ScheduledEventWithPercent::percentComplete() const
{
	float totalSteps = m_step - m_startStep;
	float elapsedSteps = simulation::step - m_startStep;
	return (elapsedSteps / totalSteps) * 100;
}

void eventSchedule::schedule(std::unique_ptr<ScheduledEvent> scheduledEvent)
{
	data[scheduledEvent->m_step].push_back(std::move(scheduledEvent));
}
void eventSchedule::schedule(std::unique_ptr<ScheduledEventWithPercent> scheduledEvent)
{
	data[scheduledEvent->m_step].push_back(std::move(scheduledEvent));
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
void eventSchedule::unschedule(const ScheduledEvent& scheduledEvent)
{
	// Unscheduling an event which is scheduled for the current step could mean modifing the event schedule while it is being iterated. Don't allow.
	assert(scheduledEvent.m_step != simulation::step);
	assert(data.contains(scheduledEvent.m_step));
	// TODO: optimize.
	data.at(scheduledEvent.m_step).remove_if([&](auto& eventPtr){ return &*eventPtr == &scheduledEvent; });
}
void eventSchedule::execute(uint32_t stepNumber)
{
	auto found = data.find(stepNumber);
	if(found == data.end())
		return;
	for(std::unique_ptr<ScheduledEvent>& scheduledEvent : found->second)
		scheduledEvent->execute();
	data.erase(stepNumber);
}
void eventSchedule::clear() { data.clear(); }

template<class EventType>
template<typename ...Args>
void HasScheduledEvent<EventType>::schedule(Args& ...args)
{
	assert(m_event == nullptr);
	std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<EventType>(args...);
	m_event = event.get();
	eventSchedule::schedule(std::move(event));
}
template<class EventType>
void HasScheduledEvent<EventType>::unschedule()
{
	assert(m_event != nullptr);
	m_event->cancel();
	m_event = nullptr;
}
template<class EventType>
void HasScheduledEvent<EventType>::maybeUnschedule()
{
	if(m_event != nullptr)
		unschedule();
}
template<class EventType>
void HasScheduledEvent<EventType>::clearPointer()
{
	assert(m_event != nullptr);
	m_event = nullptr;
}
template<class EventType>
uint32_t HasScheduledEvent<EventType>::percentComplete() const
{
	assert(m_event != nullptr);
	return m_event->percentComplete();
}
template<class EventType>
bool HasScheduledEvent<EventType>::exists() const
{
	return m_event != nullptr;
}
template<class EventType>
uint32_t HasScheduledEvent<EventType>::remainingSteps() const
{
	assert(m_event != nullptr);
	return m_event->m_step - simulation::step;
}
template<class EventType>
HasScheduledEvent<EventType>::~HasScheduledEvent()
{
	if(m_event != nullptr)
		m_event->cancel();
}
template<class EventType>
uint32_t HasScheduledEventPausable<EventType>::percentComplete() const
{
	uint32_t output = m_percent;
	auto& event = HasScheduledEvent<EventType>::m_event;
	if(!event.empty())
	{
		if(m_percent != 0)
			output += (event.percentComplete() * (100u - m_percent)) / 100u;
		else
			output = event.percentComplete();
	}
	return output;
}
template<class EventType>
void HasScheduledEventPausable<EventType>::pause()
{
	m_percent = percentComplete();
	HasScheduledEvent<EventType>::unschedule();
}
template<class EventType>
void HasScheduledEventPausable<EventType>::reset()
{
	assert(HasScheduledEvent<EventType>::m_event == nullptr);
	m_percent = 0;
}
template<class EventType>
template<typename ...Args>
void HasScheduledEventPausable<EventType>::schedule(uint32_t delay, Args& ...args)
{
	delay -= util::scaleByPercent(delay, m_percent);
	HasScheduledEvent<EventType>::schedule(delay, args...);
}
