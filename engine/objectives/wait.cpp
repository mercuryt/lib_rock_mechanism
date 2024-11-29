#include "actors/actors.h"
#include "simulation.h"
#include "types.h"
#include "wait.h"
#include "objective.h"
#include "area.h"
WaitObjective::WaitObjective(Area& area, const Step& duration, const ActorIndex& actor) :
	Objective(Priority::create(0)), m_event(area.m_eventSchedule)
{
	m_event.schedule(duration, area, *this, actor);
}
WaitObjective::WaitObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_event(area.m_eventSchedule)
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
void WaitObjective::execute(Area& area, const ActorIndex& actor) { area.getActors().objective_complete(actor, *this); }
void WaitObjective::reset(Area& area, const ActorIndex& actor) 
{ 
	m_event.maybeUnschedule(); 
	area.getActors().canReserve_clearAll(actor); 
}
WaitScheduledEvent::WaitScheduledEvent(const Step& delay, Area& area, WaitObjective& wo, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_objective(wo)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}

void WaitScheduledEvent::execute(Simulation&, Area* area) { m_objective.execute(*area, m_actor.getIndex(area->getActors().m_referenceData)); }