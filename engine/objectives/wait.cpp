#include "actors/actors.h"
#include "simulation.h"
#include "wait.h"
#include "objective.h"
#include "area.h"
WaitObjective::WaitObjective(Area& area, ActorIndex a, Step duration) :
	Objective(a, 0), m_event(area.m_eventSchedule)
{
	m_event.schedule(duration, area, *this);
}
/*
WaitObjective::WaitObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_event(deserializationMemo.m_simulation.m_eventSchedule)
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
*/
void WaitObjective::execute(Area& area) { area.getActors().objective_complete(m_actor, *this); }
void WaitObjective::reset(Area& area) 
{ 
	m_event.maybeUnschedule(); 
	area.getActors().canReserve_clearAll(m_actor); 
}
WaitScheduledEvent::WaitScheduledEvent(Step delay, Area& area, WaitObjective& wo, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_objective(wo) { }
