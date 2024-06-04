#include "goTo.h"
#include "../config.h"
#include "../actor.h"
#include "../input.h"
#include "../objective.h"
#include "../simulation.h"
#include <memory>
#include <unordered_set>
GoToObjective::GoToObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo),
	m_location(deserializationMemo.m_simulation.getBlockForJsonQuery(data["location"])) { }
Json GoToObjective::toJson() const 
{ 
	Json data = Objective::toJson();
	data["location"] = m_location;
	return data;
}
void GoToObjective::execute()
{
	if(m_actor.m_location != m_location)
		// BlockIndex, detour, adjacent, unreserved, reserve
		m_actor.m_canMove.setDestination(m_location, m_detour, false, false, false);
	else
		m_actor.m_hasObjectives.objectiveComplete(*this);
}
/*
GoToInputAction::GoToInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex& b) : 
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
