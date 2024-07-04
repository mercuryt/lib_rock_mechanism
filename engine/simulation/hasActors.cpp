#include "hasActors.h"
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
