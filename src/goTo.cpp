#include "config.h"
#include "goTo.h"
#include "block.h"
#include "objective.h"
#include "simulation.h"
GoToObjective::GoToObjective(const Json& data, DeserilizationMemo& deserilizationMemo) : Objective(data, deserilizationMemo),
	m_location(deserilizationMemo.m_simulation.getBlockForJsonQuery(data["location"])) { }
Json GoToObjective::toJson() const { return Json{{"location", m_location.positionToJson()}}; }
void GoToObjective::execute()
{
	if(m_actor.m_location != &m_location)
		// Block, detour, adjacent, unreserved, reserve
		m_actor.m_canMove.setDestination(m_location, m_detour, false, false, false);
	else
		m_actor.m_hasObjectives.objectiveComplete(*this);
}
