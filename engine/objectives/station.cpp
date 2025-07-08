#include "station.h"
#include "../simulation/simulation.h"
#include "../area/area.h"
#include "../actors/actors.h"
// Input
/*
StationInputAction::StationInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Point3D& b) :
	InputAction(actors, emplacementType, inputQueue), m_point(b) { }
void StationInputAction::execute()
{
	for(Actor* actor : m_actors)
	{
		std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(*actor, m_point);
		insertObjective(std::move(objective), *actor);
	}
}
*/
// Objective
StationObjective::StationObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo),
	m_location(data["point"].get<Point3D>()) { }
void StationObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(actors.getCombinedLocation(actor) != m_location)
		// Point3D, detour, adjacent, unreserved, reserve
		actors.move_setDestination(actor, m_location, m_detour, false, false, false);
}
void StationObjective::reset(Area& area, const ActorIndex& actor) { area.getActors().canReserve_clearAll(actor); }
