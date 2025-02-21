#include "eventSchedule.h"
#include "simulation/simulation.h"
#include "types.h"
#include "util.h"
#include "area/area.h"
#include <cassert>
ScheduledEvent::ScheduledEvent(Simulation& simulation, const Step& delay, const Step start) :
	m_startStep(start.empty() ? simulation.m_step : start), m_step(m_startStep + delay)
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
	return Step::create(m_step.get() - simulation.m_step.get());
}
Step ScheduledEvent::elapsedSteps(Simulation& simulation) const { return simulation.m_step - m_startStep; }
Percent ScheduledEvent::percentComplete(Simulation& simulation) const
{
	return Percent::create(fractionComplete(simulation) * 100u);
}
float ScheduledEvent::fractionComplete(Simulation& simulation) const
{
	assert(m_step >= m_startStep);
	Step totalSteps = m_step - m_startStep;
	Step elapsedSteps = simulation.m_step - m_startStep;
	return (float)elapsedSteps.get() / (float)totalSteps.get();
}

Step ScheduledEvent::duration() const { return Step::create(m_step.get() - m_startStep.get()); }
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
void EventSchedule::doStep(const Step& stepNumber)
{
	if(m_data.begin()->first > stepNumber)
		return;
	assert(m_data.begin()->first == stepNumber);
	for(std::unique_ptr<ScheduledEvent>& scheduledEvent : m_data.begin()->second)
		if(!scheduledEvent->m_cancel)
		{
			// Clear references first so events can reschedule themselves in the same slots.
			scheduledEvent->clearReferences(m_simulation, m_area);
			scheduledEvent->execute(m_simulation, m_area);
		}
	m_data.erase(m_data.begin());
}
void EventSchedule::clear()
{
	for(auto& [step, events] : m_data)
		for(std::unique_ptr<ScheduledEvent>& scheduledEvent : events)
			if(!scheduledEvent->m_cancel)
				scheduledEvent->clearReferences(m_simulation, m_area);
}
Step EventSchedule::getNextEventStep() const
{
	if(m_data.empty())
		return Step::null();
	return m_data.begin()->first;
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
