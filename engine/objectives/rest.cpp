#include "rest.h"
#include "../objective.h"
#include "../simulation.h"
#include "../area.h"
#include "../config.h"
#include "actors/actors.h"
#include "types.h"

RestObjective::RestObjective(Area& area) : Objective(Priority::create(0)), m_restEvent(area.m_eventSchedule) { }
RestObjective::RestObjective(const Json& data, Area& area, const ActorIndex& actor) : Objective(data), m_restEvent(area.m_eventSchedule) 
{
	if(data.contains("eventStart"))
		m_restEvent.schedule(area, *this, actor, data["eventStart"].get<Step>());
}
Json RestObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_restEvent.exists())
		data["eventStart"] = m_restEvent.getStartStep();
	return data;
}
	
void RestObjective::execute(Area& area, const ActorIndex& actor) { m_restEvent.schedule(area, *this, actor); }
void RestObjective::reset(Area& area, const ActorIndex& actor) 
{ 
	cancel(area, actor); 
	area.getActors().canReserve_clearAll(actor);
}
RestEvent::RestEvent(Area& area, RestObjective& ro, const ActorIndex& actor, Step start) :
	ScheduledEvent(area.m_simulation, Config::restIntervalSteps, start), m_objective(ro)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
void RestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex();
	actors.stamina_recover(actor);
	actors.objective_complete(actor, m_objective);
}
