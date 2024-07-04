#include "eventSchedule.h"
#include "simulation.h"
#include "util.h"
#include "area.h"
#include <cassert>
ScheduledEvent::ScheduledEvent(Simulation& simulation, const Step delay, const Step start) :
	m_startStep(start == 0 ? simulation.m_step : start), m_step(m_startStep + delay)
{
	assert(delay != 0);
	assert(simulation.m_step <= m_step);
}
void ScheduledEvent::cancel(Simulation& simulation, Area* area) 
{
	onCancel(simulation, area);
	if(area == nullptr)
		simulation.m_eventSchedule.unschedule(*this); 
	else
		area->m_eventSchedule.unschedule(*this);
}
Step ScheduledEvent::remaningSteps(Simulation& simulation) const 
{ 
	assert(m_step > simulation.m_step);
	return m_step - simulation.m_step;
}
Step ScheduledEvent::elapsedSteps(Simulation& simulation) const { return simulation.m_step - m_startStep; }
Percent ScheduledEvent::percentComplete(Simulation& simulation) const
{
	return fractionComplete(simulation) * 100u;
}
float ScheduledEvent::fractionComplete(Simulation& simulation) const
{
	assert(m_step >= m_startStep);
	Step totalSteps = m_step - m_startStep;
	Step elapsedSteps = simulation.m_step - m_startStep;
	return (float)elapsedSteps / (float)totalSteps;
}

Step ScheduledEvent::duration() const { return m_step - m_startStep ; }
void EventSchedule::schedule(std::unique_ptr<ScheduledEvent> scheduledEvent)
{
	m_data[scheduledEvent->m_step].push_back(std::move(scheduledEvent));
}
void EventSchedule::unschedule(ScheduledEvent& scheduledEvent)
{
	assert(m_data.contains(scheduledEvent.m_step));
	scheduledEvent.m_cancel = true;
	scheduledEvent.clearReferences(m_simulation, m_area);
}
void EventSchedule::doStep(Step stepNumber)
{
	assert(m_data.begin()->first >= stepNumber);
	auto found = m_data.find(stepNumber);
	if(found == m_data.end())
		return;
	for(std::unique_ptr<ScheduledEvent>& scheduledEvent : found->second)
		if(!scheduledEvent->m_cancel)
		{
			// Clear references first so events can reschedule themselves in the same slots.
			scheduledEvent->clearReferences(m_simulation, m_area);
			scheduledEvent->execute(m_simulation, m_area);
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
Step EventSchedule::simulationStep() const { return m_simulation.m_step; }
