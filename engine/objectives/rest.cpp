#include "rest.h"
#include "../objective.h"
#include "../simulation.h"
#include "../area.h"
#include "../config.h"
#include "actors/actors.h"

RestObjective::RestObjective(Area& area) : Objective(0), m_restEvent(area.m_eventSchedule) { }
RestObjective::RestObjective(const Json& data, Area& area) : Objective(data), m_restEvent(area.m_eventSchedule) 
{
	if(data.contains("eventStart"))
		m_restEvent.schedule(*this, data["eventStart"].get<Step>());
}
Json RestObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_restEvent.exists())
		data["eventStart"] = m_restEvent.getStartStep();
	return data;
}
	
void RestObjective::execute(Area& area, ActorIndex) { m_restEvent.schedule(area.m_simulation, *this); }
void RestObjective::reset(Area& area, ActorIndex actor) 
{ 
	cancel(area, actor); 
	area.getActors().canReserve_clearAll(actor);
}
RestEvent::RestEvent(Simulation& simulation, RestObjective& ro, const Step start) :
	ScheduledEvent(simulation, Config::restIntervalSteps, start), m_objective(ro) { }
void RestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex();
	actors.stamina_recover(actor);
	actors.objective_complete(actor, m_objective);
}
