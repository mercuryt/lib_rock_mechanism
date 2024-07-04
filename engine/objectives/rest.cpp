#include "rest.h"
#include "../objective.h"
#include "../simulation.h"
#include "../area.h"
#include "../config.h"
#include "actors/actors.h"

RestObjective::RestObjective(Area& area, ActorIndex a) : Objective(a, 0), m_restEvent(area.m_eventSchedule) { }
/*
RestObjective::RestObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_restEvent(deserializationMemo.m_simulation.m_eventSchedule) 
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
*/	
	
void RestObjective::execute(Area& area) { m_restEvent.schedule(area.m_simulation, *this); }
void RestObjective::reset(Area& area) 
{ 
	cancel(area); 
	area.getActors().canReserve_clearAll(m_actor);
}
RestEvent::RestEvent(Simulation& simulation, RestObjective& ro, const Step start) :
	ScheduledEvent(simulation, Config::restIntervalSteps, start), m_objective(ro) { }
void RestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	actors.stamina_recover(m_objective.m_actor);
	actors.objective_complete(m_objective.m_actor, m_objective);
}
