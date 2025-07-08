#include "goTo.h"
#include "../config.h"
#include "../objective.h"
#include "../simulation/simulation.h"
#include "../area/area.h"
#include "../path/terrainFacade.hpp"
#include "../actors/actors.h"
#include "../numericTypes/types.h"
GoToObjective::GoToObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_location(data["location"].get<Point3D>()) { }
Json GoToObjective::toJson() const
{
	Json data = Objective::toJson();
	data["location"] = m_location;
	return data;
}
void GoToObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(actors.getCombinedLocation(actor) != m_location)
		// Point3D, detour, adjacent, unreserved, reserve
		actors.move_setDestination(actor, m_location, m_detour, false, false, false);
	else
		actors.objective_complete(actor, *this);
}