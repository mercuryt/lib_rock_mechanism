#include "actors/actors.h"
#include "simulation.h"
#include "types.h"
#include "wait.h"
#include "objective.h"
#include "area.h"
WaitObjective::WaitObjective(Area& area, Step duration, ActorIndex actor) :
	Objective(Priority::create(0)), m_event(area.m_eventSchedule)
{

	m_event.schedule(duration, area, *this, actor);
}
WaitObjective::WaitObjective(const Json& data, Area& area, ActorIndex actor) :
	Objective(data), m_event(area.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_event.schedule(data["duration"].get<Step>(), area, *this, actor, data["eventStart"].get<Step>());
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
void WaitObjective::execute(Area& area, ActorIndex actor) { area.getActors().objective_complete(actor, *this); }
void WaitObjective::reset(Area& area, ActorIndex actor) 
{ 
	m_event.maybeUnschedule(); 
	area.getActors().canReserve_clearAll(actor); 
}
WaitScheduledEvent::WaitScheduledEvent(Step delay, Area& area, WaitObjective& wo, ActorIndex actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_objective(wo)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
