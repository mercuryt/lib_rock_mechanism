#include "actors/actors.h"
#include "simulation.h"
#include "wait.h"
#include "objective.h"
#include "area.h"
WaitObjective::WaitObjective(Area& area, Step duration) :
	Objective(0), m_event(area.m_eventSchedule)
{
	m_event.schedule(duration, area, *this);
}
WaitObjective::WaitObjective(const Json& data, Area& area) :
	Objective(data), m_event(area.m_eventSchedule)
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
void WaitObjective::execute(Area& area) { area.getActors().objective_complete(m_actor, *this); }
void WaitObjective::reset(Area& area) 
{ 
	m_event.maybeUnschedule(); 
	area.getActors().canReserve_clearAll(m_actor); 
}
WaitScheduledEvent::WaitScheduledEvent(Step delay, Area& area, WaitObjective& wo, ActorReference actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(actor), m_objective(wo) { }
