#include "exterminate.h"
#include "../simulation.h"
#include "../block.h"
#include "../actor.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "eventSchedule.h"
ExterminateObjective::ExterminateObjective(Actor& a, Block& destination) : Objective(a, Config::exterminatePriority), m_destination(destination), m_event(a.getEventSchedule()) { }
ExterminateObjective::ExterminateObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo),
	m_destination(deserializationMemo.blockReference(data["destination"])),
	m_event(deserializationMemo.m_simulation.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_event.schedule(*this, data["eventStart"].get<Step>());
}
Json ExterminateObjective::toJson() const
{
	Json output = Objective::toJson();
	output["destination"] = m_destination;
	if(m_event.exists())
		output["eventStart"] = m_event.getStartStep();
	return output;
}
void ExterminateObjective::execute()
{
	Actor* closest = nullptr;
	for(Actor* actor : m_actor.m_canSee.getCurrentlyVisibleActors())
	{
		if(
			actor->isSentient() &&
			(!closest || closest->m_location->taxiDistance(*m_actor.m_location) < actor->m_location->taxiDistance(*m_actor.m_location)) && 
			!actor->isAlly(m_actor)
		)
			closest = actor;
	}
	if(closest)
		m_actor.m_canFight.setTarget(*closest);
	else
	{
		static constexpr DistanceInBlocks distanceToRallyPoint = 10;
		if(m_actor.m_location->taxiDistance(m_destination) > distanceToRallyPoint)
			m_actor.m_canMove.setDestination(m_destination);
		m_event.schedule(*this);
	}
}
ExterminateObjectiveScheduledEvent::ExterminateObjectiveScheduledEvent(ExterminateObjective& o, Step start) : 
	ScheduledEvent(o.m_destination.m_area->m_simulation, Config::exterminateCheckFrequency, start),
	m_objective(o) { }
