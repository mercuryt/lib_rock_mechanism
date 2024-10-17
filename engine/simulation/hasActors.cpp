#include "hasActors.h"
#include "actors/actors.h"
#include "deserializationMemo.h"
void SimulationHasActors::registerActor(const ActorId& id, Actors& store, const ActorIndex& index)
{
	assert(!m_actors.contains(id));
	m_actors.try_emplace(id, &store, index);
}
void SimulationHasActors::removeActor(const ActorId& id)
{
	assert(m_actors.contains(id));
	m_actors.erase(id);
}
ActorIndex SimulationHasActors::getIndexForId(const ActorId& id) const
{
	return m_actors.at(id).index;
}
Area& SimulationHasActors::getAreaForId(const ActorId& id) const
{
	return m_actors.at(id).store->getArea();
}
