#include "actorQuery.h"
#include "simulation.h"
#include "simulation/hasActors.h"
#include "types.h"
#include "area.h"
#include "actors.h"
// ActorQuery, to be used to search for actors.
bool ActorQuery::query(Area& area, const ActorIndex& other) const
{
	Actors& actors = area.getActors();
	if(actor.exists())
		return actor.getIndex(actors.m_referenceData) == other;
	if(carryWeight != 0 && actors.canPickUp_getMass(other) < carryWeight)
		return false;
	if(checkIfSentient && actors.isSentient(other) != sentient)
		return false;
	return true;
}
ActorQuery ActorQuery::makeFor(const ActorReference& a) { return ActorQuery(a); }
ActorQuery ActorQuery::makeForCarryWeight(const Mass& cw) { ActorQuery output; output.carryWeight = cw; return output; }
