#include "hasActors.h"
#include "actors/actors.h"
#include "deserializationMemo.h"
void SimulationHasActors::registerActor(ActorId id, Actors& store, ActorIndex index)
{
	assert(!m_actors.contains(id));
	auto [iter, result] = m_actors.try_emplace(id, store, index);
	assert(result);
}
void SimulationHasActors::removeActor(ActorId id)
{
	assert(m_actors.contains(id));
	m_actors.erase(id);
}
ActorIndex SimulationHasActors::getIndexForId(ActorId id) const
{
	return m_actors.at(id).index;
}
Area& SimulationHasActors::getAreaForId(ActorId id) const
{
	return m_actors.at(id).store.getArea();
}
