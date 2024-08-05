#include "exterminate.h"
#include "../simulation.h"
#include "../actors/actors.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "blocks/blocks.h"
#include "types.h"
ExterminateObjective::ExterminateObjective(Area& area, BlockIndex destination) :
	Objective(Config::exterminatePriority), m_destination(destination), m_event(area.m_eventSchedule) { }
ExterminateObjective::ExterminateObjective(const Json& data, Area& area, ActorIndex actor) : 
	Objective(data),
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
void ExterminateObjective::execute(Area& area, ActorIndex actor)
{
	ActorIndex closest;
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	BlockIndex thisActorLocation = actors.getLocation(actor);
	BlockIndex closestActorLocation;
	for(ActorIndex actor : actors.vision_getCanSee(actor))
	{
		BlockIndex location = actors.getLocation(actor);
		if(
			actors.hasFaction(actor) &&
			!actors.isAlly(actor, actor) &&
			(closest.empty() || blocks.taxiDistance(closestActorLocation, thisActorLocation) < blocks.taxiDistance(location, thisActorLocation))
		)
		{
			closest = actor;
			closestActorLocation = location;
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
ExterminateObjectiveScheduledEvent::ExterminateObjectiveScheduledEvent(Area& area, ExterminateObjective& o, ActorIndex actor, Step start) : 
	ScheduledEvent(area.m_simulation, Config::exterminateCheckFrequency, start),
	m_objective(o)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
