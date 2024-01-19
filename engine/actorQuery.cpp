#include "actorQuery.h"
#include "simulation.h"
ActorQuery::ActorQuery(const Json& data, DeserializationMemo& deserializationMemo) :
	actor(data.contains("actor") ? &deserializationMemo.m_simulation.getActorById(data["actor"].get<ActorId>()) : nullptr),
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
// ActorQuery, to be used to search for actors.
bool ActorQuery::operator()(Actor& other) const
{
	if(actor != nullptr)
		return actor == &other;
	if(carryWeight != 0 && other.m_canPickup.getMass() < carryWeight)
		return false;
	if(checkIfSentient && other.isSentient() != sentient)
		return false;
	return true;
}
ActorQuery ActorQuery::makeFor(Actor& a) { return ActorQuery(a); }
ActorQuery ActorQuery::makeForCarryWeight(Mass cw) { return ActorQuery(cw, false, false); }

