#include "rest.h"
#include "../actor.h"
#include "../objective.h"
#include "../simulation.h"

RestObjective::RestObjective(Actor& a) : Objective(a, 0), m_restEvent(a.getEventSchedule()) { }
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
void RestObjective::reset() 
{ 
	cancel(); 
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
RestEvent::RestEvent(RestObjective& ro, const Step start) : ScheduledEvent(ro.m_actor.getSimulation(), Config::restIntervalSteps, start), m_objective(ro) { }
void RestEvent::execute()
{
	m_objective.m_actor.m_stamina.recover();
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
