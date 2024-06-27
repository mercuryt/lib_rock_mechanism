#include "actorQuery.h"
#include "simulation.h"
#include "simulation/hasActors.h"
#include "types.h"
#include "area.h"
/*
ActorQuery::ActorQuery(const Json& data, DeserializationMemo& deserializationMemo) :
	actor(data.contains("actor") ? &deserializationMemo.m_simulation.m_actors->getById(data["actor"].get<ActorId>()) : nullptr),
	carryWeight(data.contains("carryWeight") ? data["carryWeight"].get<Mass>() : 0),
	checkIfSentient(data.contains("checkIfSentient")),
	sentient(data.contains("sentient")) { }
Json ActorQuery::toJson() const
{
	Json data;
	if(actor)
		data["actor"] = actor->m_id;
	if(carryWeight)
		data["carryWeight"] = carryWeight;
	if(checkIfSentient)
		data["checkIfSentient"] = checkIfSentient;
	if(sentient)
		data["sentient"] = sentient;
	return data;
}
*/
// ActorQuery, to be used to search for actors.
bool ActorQuery::query(Area& area, ActorIndex other) const
{
	if(actor != ACTOR_INDEX_MAX)
		return actor == other;
	Actors& actors = area.m_actors;
	if(carryWeight != 0 && actors.canPickUp_getMass(other) < carryWeight)
		return false;
	if(checkIfSentient && actors.isSentient(other) != sentient)
		return false;
	return true;
}
ActorQuery ActorQuery::makeFor(ActorIndex a) { return ActorQuery(a); }
ActorQuery ActorQuery::makeForCarryWeight(Mass cw) { return ActorQuery(cw, false, false); }
