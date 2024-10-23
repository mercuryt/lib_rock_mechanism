#include "actorQuery.h"
#include "simulation.h"
#include "simulation/hasActors.h"
#include "types.h"
#include "area.h"
#include "actors.h"
ActorQuery::ActorQuery(const Json& data, Area& area) :
	carryWeight(data.contains("carryWeight") ? data["carryWeight"].get<Mass>() : Mass::create(0)),
	checkIfSentient(data.contains("checkIfSentient")),
	sentient(data.contains("sentient")) 
{ 
	if(data.contains("actor"))
		actor.setTarget(area.getActors().getReferenceTarget(data["actor"].get<ActorIndex>()));
}
// ActorQuery, to be used to search for actors.
bool ActorQuery::query(Area& area, const ActorIndex& other) const
{
	if(actor.exists())
		return actor.getIndex() == other;
	Actors& actors = area.getActors();
	if(carryWeight != 0 && actors.canPickUp_getMass(other) < carryWeight)
		return false;
	if(checkIfSentient && actors.isSentient(other) != sentient)
		return false;
	return true;
}
ActorQuery ActorQuery::makeFor(const ActorReference& a) { return ActorQuery(a); }
ActorQuery ActorQuery::makeForCarryWeight(const Mass& cw) { return ActorQuery(cw, false, false); }
