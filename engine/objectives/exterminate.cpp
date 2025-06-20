#include "exterminate.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../deserializationMemo.h"
#include "blocks/blocks.h"
#include "numericTypes/types.h"
ExterminateObjective::ExterminateObjective(Area& area, const BlockIndex& destination) :
	Objective(Config::exterminatePriority), m_destination(destination), m_event(area.m_eventSchedule) { }
ExterminateObjective::ExterminateObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_destination(data["destination"].get<BlockIndex>()),
	m_event(area.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_event.schedule(area, *this, actor, data["eventStart"].get<Step>());
}
Json ExterminateObjective::toJson() const
{
	Json output = Objective::toJson();
	output["destination"] = m_destination;
	if(m_event.exists())
		output["eventStart"] = m_event.getStartStep();
	return output;
}
void ExterminateObjective::execute(Area& area, const ActorIndex& actor)
{
	ActorIndex closest;
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	BlockIndex thisActorLocation = actors.getLocation(actor);
	BlockIndex closestActorLocation;
	for(const ActorReference& other : actors.vision_getCanSee(actor))
	{
		ActorIndex otherIndex = other.getIndex(actors.m_referenceData);
		BlockIndex otherLocation = actors.getLocation(otherIndex);
		if(
			actors.hasFaction(otherIndex) &&
			!actors.isAlly(actor, otherIndex) &&
			(closest.empty() || blocks.taxiDistance(closestActorLocation, thisActorLocation) < blocks.taxiDistance(otherLocation, thisActorLocation))
		)
		{
			closest = otherIndex;
			closestActorLocation = otherLocation;
		}
	}
	if(closest.exists())
		actors.combat_setTarget(actor, closest);
	else
	{
		static constexpr DistanceInBlocks distanceToRallyPoint = DistanceInBlocks::create(10);
		if(blocks.taxiDistance(thisActorLocation, m_destination) > distanceToRallyPoint)
			actors.move_setDestination(actor, m_destination, m_detour);
		m_event.schedule(area, *this, actor);
	}
}
ExterminateObjectiveScheduledEvent::ExterminateObjectiveScheduledEvent(Area& area, ExterminateObjective& o, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, Config::exterminateCheckFrequency, start),
	m_objective(o)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void ExterminateObjectiveScheduledEvent::execute(Simulation&, Area* area) { m_objective.execute(*area, m_actor.getIndex(area->getActors().m_referenceData)); }