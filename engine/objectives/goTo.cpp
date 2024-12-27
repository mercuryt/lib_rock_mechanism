#include "goTo.h"
#include "../config.h"
#include "../input.h"
#include "../objective.h"
#include "../simulation.h"
#include "../area.h"
#include "../terrainFacade.hpp"
#include "../actors/actors.h"
#include "../types.h"
GoToObjective::GoToObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo),
	m_location(data["location"].get<BlockIndex>()) { }
Json GoToObjective::toJson() const 
{ 
	Json data = Objective::toJson();
	data["location"] = m_location;
	return data;
}
void GoToObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(actors.getLocation(actor) != m_location)
		// BlockIndex, detour, adjacent, unreserved, reserve
		actors.move_setDestination(actor, m_location, m_detour, false, false, false);
	else
		actors.objective_complete(actor, *this);
}
/*
GoToInputAction::GoToInputAction(ActorIndices actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex& b) : 
	InputAction(actors, emplacementType, inputQueue), m_block(b) { }
void GoToInputAction::execute()
{
	for(Actor* actor : m_actors)
	{
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(*actor, m_block);
		insertObjective(std::move(objective), *actor);
	}
}
*/
