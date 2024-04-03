#include "station.h"
#include "block.h"
// Input
StationInputAction::StationInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Block& b) : 
	InputAction(actors, emplacementType, inputQueue), m_block(b) { }
void StationInputAction::execute()
{
	for(Actor* actor : m_actors)
	{
		std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(*actor, m_block);
		insertObjective(std::move(objective), *actor);
	}
}
// Objective
void StationObjective::execute()
{
	if(m_actor.m_location != &m_location)
		// Block, detour, adjacent, unreserved, reserve
		m_actor.m_canMove.setDestination(m_location, m_detour, false, false, false);
}
void StationObjective::reset() { m_actor.m_canReserve.deleteAllWithoutCallback(); }
