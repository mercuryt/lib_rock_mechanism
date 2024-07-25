#include "station.h"
#include "../simulation.h"
#include "../area.h"
#include "../actors/actors.h"
// Input
/*
StationInputAction::StationInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex& b) : 
	InputAction(actors, emplacementType, inputQueue), m_block(b) { }
void StationInputAction::execute()
{
	for(Actor* actor : m_actors)
	{
		std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(*actor, m_block);
		insertObjective(std::move(objective), *actor);
	}
}
*/
// Objective
StationObjective::StationObjective(const Json& data) : Objective(data), 
	m_location(data["block"].get<BlockIndex>()) { }
void StationObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(actors.getLocation(actor) != m_location)
		// BlockIndex, detour, adjacent, unreserved, reserve
		actors.move_setDestination(actor, m_location, m_detour, false, false, false);
}
void StationObjective::reset(Area& area, ActorIndex actor) { area.getActors().canReserve_clearAll(actor); }
