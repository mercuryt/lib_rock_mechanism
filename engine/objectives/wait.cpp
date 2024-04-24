#include "simulation.h"
#include "wait.h"
#include "actor.h"
#include "objective.h"
WaitObjective::WaitObjective(Actor& a, Step duration) : Objective(a, 0), m_event(a.getEventSchedule())
{
	m_event.schedule(duration, *this);
}
WaitObjective::WaitObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_event(deserializationMemo.m_simulation.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_event.schedule(data["duration"].get<Step>(), *this, data["eventStart"].get<Step>());
}
Json WaitObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_event.exists())
	{
		data["eventStart"] = m_event.getStartStep();
		data["duration"] = m_event.duration();
	}
	return data;
}
void WaitObjective::execute() { m_actor.m_hasObjectives.objectiveComplete(*this); }
void WaitObjective::reset() 
{ 
	m_event.maybeUnschedule(); 
	m_actor.m_canReserve.deleteAllWithoutCallback(); 
}
WaitScheduledEvent::WaitScheduledEvent(Step delay, WaitObjective& wo, const Step start) : ScheduledEvent(wo.m_actor.getSimulation(), delay, start), m_objective(wo) { }
