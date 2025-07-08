#include "exterminate.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../deserializationMemo.h"
#include "space/space.h"
#include "numericTypes/types.h"
ExterminateObjective::ExterminateObjective(Area& area, const Point3D& destination) :
	Objective(Config::exterminatePriority), m_destination(destination), m_event(area.m_eventSchedule) { }
ExterminateObjective::ExterminateObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_destination(data["destination"].get<Point3D>()),
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
	Actors& actors = area.getActors();
	Point3D thisActorLocation = actors.getLocation(actor);
	Point3D closestActorLocation;
	for(const ActorReference& other : actors.vision_getCanSee(actor))
	{
		ActorIndex otherIndex = other.getIndex(actors.m_referenceData);
		Point3D otherLocation = actors.getLocation(otherIndex);
		if(
			actors.hasFaction(otherIndex) &&
			!actors.isAlly(actor, otherIndex) &&
			(closest.empty() || closestActorLocation.taxiDistanceTo(thisActorLocation) < otherLocation.taxiDistanceTo(thisActorLocation))
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
		static constexpr Distance distanceToRallyPoint = Distance::create(10);
		if(thisActorLocation.taxiDistanceTo(m_destination) > distanceToRallyPoint)
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