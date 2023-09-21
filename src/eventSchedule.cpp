#include "eventSchedule.h"
#include "simulation.h"
#include "util.h"
#include "area.h"
#include <cassert>
ScheduledEvent::ScheduledEvent(Simulation& simulation, const Step delay) : m_simulation(simulation), m_step(delay + simulation.m_step), m_cancel(false) 
{
	assert(delay != 0);
}
void ScheduledEvent::cancel() 
{
	onCancel();
       	m_simulation.m_eventSchedule.unschedule(*this); 
}
Step ScheduledEvent::remaningSteps() const { return m_step - m_simulation.m_step; }

ScheduledEventWithPercent::ScheduledEventWithPercent(Simulation& simulation, const Step delay) : ScheduledEvent(simulation, delay), m_startStep(simulation.m_step) {}
Percent ScheduledEventWithPercent::percentComplete() const
{
	Step totalSteps = m_step - m_startStep;
	Step elapsedSteps = m_simulation.m_step - m_startStep;
	return ((float)elapsedSteps / (float)totalSteps) * 100u;
}

Step ScheduledEventWithPercent::duration() const { return m_step - m_startStep ; }
void EventSchedule::schedule(std::unique_ptr<ScheduledEvent> scheduledEvent)
{
	m_data[scheduledEvent->m_step].push_back(std::move(scheduledEvent));
}
void EventSchedule::schedule(std::unique_ptr<ScheduledEventWithPercent> scheduledEvent)
{
	m_data[scheduledEvent->m_step].push_back(std::move(scheduledEvent));
}
void EventSchedule::unschedule(ScheduledEvent& scheduledEvent)
{
	assert(m_data.contains(scheduledEvent.m_step));
	scheduledEvent.m_cancel = true;
	scheduledEvent.clearReferences();
}
void EventSchedule::execute(Step stepNumber)
{
	assert(m_data.begin()->first >= stepNumber);
	auto found = m_data.find(stepNumber);
	if(found == m_data.end())
		return;
	for(std::unique_ptr<ScheduledEvent>& scheduledEvent : found->second)
		if(!scheduledEvent->m_cancel)
		{
			// Clear references first so events can reschedule themselves in the same slots.
			scheduledEvent->clearReferences();
			scheduledEvent->execute();
		}
	m_data.erase(stepNumber);
}
uint32_t EventSchedule::count()
{
	uint32_t output = 0;
	for(auto& pair : m_data)
		for(auto& event : pair.second)
			if(!event->m_cancel)
				output++;
	return output;
}
