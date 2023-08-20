#include "eventSchedule.h"
#include "simulation.h"
#include "util.h"
#include "area.h"
#include <cassert>
ScheduledEvent::ScheduledEvent(const Step delay) : m_step(delay + simulation::step), m_cancel(false) 
{
	assert(delay > 0);
}
void ScheduledEvent::cancel() { eventSchedule::unschedule(*this); }
Step ScheduledEvent::remaningSteps() const { return m_step - simulation::step; }

ScheduledEventWithPercent::ScheduledEventWithPercent(const Step delay) : ScheduledEvent(delay), m_startStep(simulation::step) {}
uint32_t ScheduledEventWithPercent::percentComplete() const
{
	Step totalSteps = m_step - m_startStep;
	Step elapsedSteps = simulation::step - m_startStep;
	return ((float)elapsedSteps / (float)totalSteps) * 100u;
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
void eventSchedule::unschedule(ScheduledEvent& scheduledEvent)
{
	assert(data.contains(scheduledEvent.m_step));
	scheduledEvent.m_cancel = true;
	scheduledEvent.clearReferences();
}
void eventSchedule::execute(Step stepNumber)
{
	auto found = data.find(stepNumber);
	if(found == data.end())
		return;
	for(std::unique_ptr<ScheduledEvent>& scheduledEvent : found->second)
		if(!scheduledEvent->m_cancel)
		{
			// Clear references first so events can reschedule themselves in the same slots.
			scheduledEvent->clearReferences();
			scheduledEvent->execute();
		}
	data.erase(stepNumber);
}
// Only safe to use if also flushing everything else.
void eventSchedule::clear() { data.clear(); }
uint32_t eventSchedule::count()
{
	uint32_t output = 0;
	for(auto& pair : data)
		output += pair.second.size();
	return output;
}
