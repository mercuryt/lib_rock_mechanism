#include "rest.h"
#include "../objective.h"
#include "../simulation/simulation.h"
#include "../area/area.h"
#include "../config.h"
#include "actors/actors.h"
#include "types.h"

RestObjective::RestObjective(Area& area) : Objective(Priority::create(0)), m_restEvent(area.m_eventSchedule) { }
RestObjective::RestObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_restEvent(area.m_eventSchedule)
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
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void RestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	actors.stamina_recover(actor);
	actors.objective_complete(actor, m_objective);
}
