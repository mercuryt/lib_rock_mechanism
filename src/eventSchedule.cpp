#include "eventSchedule.h"
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

void EventSchedule::schedule(std::unique_ptr<ScheduledEvent> scheduledEvent)
{
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
void EventSchedule::unschedule(const ScheduledEvent& scheduledEvent)
{
	// Unscheduling an event which is scheduled for the current step could mean modifing the event schedule while it is being iterated. Don't allow.
	assert(scheduledEvent.m_step != simulation::step);
	assert(m_data.contains(scheduledEvent.m_step));
	// TODO: optimize.
	m_data.at(scheduledEvent.m_step).remove_if([&](auto& eventPtr){ return &*eventPtr == &scheduledEvent; });
}
void EventSchedule::execute(uint32_t stepNumber)
{
	auto found = m_data.find(stepNumber);
	if(found == m_data.end())
		return;
	for(std::unique_ptr<ScheduledEvent>& scheduledEvent : found->second)
		scheduledEvent->execute();
	m_data.erase(stepNumber);
}
void EventSchedule::clear()
{
	m_data.clear();
}

HasScheduledEvent::HasScheduledEvent() : m_event(nullptr) {}
template<typename ...Args>
void HasScheduledEvent::schedule(Args ...args)
{
	assert(m_event == nullptr);
	std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<EventType>(args...);
	m_event = event.get();
	eventSchedule::schedule(event);
}
void HasScheduledEvent::unschedule()
{
	assert(m_event != nullptr);
	m_event->cancel();
	m_event = nullptr;
}
void HasScheduledEvent::maybeUnschedule()
{
	if(m_event != nullptr)
		unschedule();
}
uint32_t HasScheduledEvent::percentComplete() const
{
	assert(m_event != nullptr);
	return m_event->percentComplete();
}
bool HasScheduledEvent::exists()
{
	return m_event != nullptr;
}
void HasScheduledEvent::clearPointer()
{
	m_event = nullptr;
}
~HasScheduledEvent::HasScheduledEvent()
{
	if(m_event != nullptr)
		m_event->cancel();
}
HasScheduledEventPausable::HasScheduledEventPausable() : HasScheduledEvent<EventType>(), m_percent(0) { }
uint32_t HasScheduledEventPausable::percentComplete() const
{
	uint32_t output = m_percent;
	auto& event = HasScheduledEvent<EventType>::m_event;
	if(!event.empty())
	{
		if(m_percent != 0)
			output += (m_event.percentComplete() * (100u - m_percent)) / 100u;
		else
			output = m_event.percentComplete();
	}
	return output;
}
void HasScheduledEventPausable::pause()
{
	m_percent = percentComplete();
	unschedule();
}
void HasScheduledEventPausable::reset()
{
	assert(m_event == nullptr);
	m_percent = 0;
}
	template<typename ...Args>
void HasScheduledEventPausable::schedule(uint32_t delay, Args ...args)
{
	delay -= util::scaleByPercent(delay, m_percent);
	HasScheduledEvent<EventType>::schedule(delay, args...);
}
