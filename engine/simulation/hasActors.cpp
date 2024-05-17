#include "hasActors.h"
#include "deserializationMemo.h"
SimulationHasActors::SimulationHasActors(const Json& data, DeserializationMemo&, Simulation& simulation) : m_simulation(simulation)
{
	m_nextId = data["nextId"].get<ActorId>();
}
Actor& SimulationHasActors::createActor(ActorParamaters params)
{
	params.simulation = &m_simulation;
	auto [iter, emplaced] = m_actors.emplace(
		params.getId(),
		params
	);
	assert(emplaced);
	return iter->second;
}
Actor& SimulationHasActors::createActor(const AnimalSpecies& species, Block& location, Percent percentGrown)
{
	return createActor(ActorParamaters{.species = species, .percentGrown = percentGrown, .location = &location});
}
void SimulationHasActors::destroyActor(Actor& actor)
{
	if(actor.m_location)
		actor.leaveArea();
	m_actors.erase(actor.m_id);
	//TODO: Destroy hibernation json file if any.
}
void SimulationHasActors::clearAll()
{
	for(auto& pair : m_actors)
	{
		pair.second.m_canReserve.deleteAllWithoutCallback();
		pair.second.m_onDestroy.unsubscribeAll();
	}
	m_actors.clear();
}
Actor& SimulationHasActors::loadActorFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	auto id = data["id"].get<ActorId>();
	m_actors.try_emplace(id, data, deserializationMemo);
	return m_actors.at(id);
}
Actor& SimulationHasActors::getById(ActorId id) 
{ 
	if(!m_actors.contains(id))
	{
		//TODO: Load from world DB.
		assert(false);
	}
	else
		return m_actors.at(id); 
	return m_actors.begin()->second;
} 
Json SimulationHasActors::toJson() const
{
	return {{"nextId", m_nextId}};
}
