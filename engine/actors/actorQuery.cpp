#include "actorQuery.h"
#include "simulation/simulation.h"
#include "simulation/hasActors.h"
#include "numericTypes/types.h"
#include "area/area.h"
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
ActorQuery ActorQuery::makeFor(const ActorReference& a) { ActorQuery output; output.actor = a; return output;}
ActorQuery ActorQuery::makeForCarryWeight(const Mass& cw) { ActorQuery output; output.carryWeight = cw; return output; }
ActorQuery::ActorQuery(const Json& data, Area& area)
{
	if(data.contains("actor"))
		actor.load(data["actor"], area.getActors().m_referenceData);
	if(data.contains("carryWeight"))
		data["carryWeight"].get_to(carryWeight);
	if(data.contains("checkIfSentient"))
		checkIfSentient = true;
	if(data.contains("sentient"))
		sentient = true;
}
Json ActorQuery::toJson() const
{
	Json output;
	if(actor.exists())
		output["actor"] = actor;
	if(carryWeight.exists())
		output["carryWeight"] = carryWeight;
	if(checkIfSentient)
		output["checkIfSentient"] = true;
	if(checkIfSentient && sentient)
		output["sentient"] = true;
	return output;
}
