#include "blocks.h"
#include "../actors/actors.h"
#include "../area.h"
#include "types.h"
void Blocks::actor_record(BlockIndex index, ActorIndex actor, CollisionVolume volume)
{
	Actors& actors = m_area.getActors();
	m_actorVolume[index].emplace_back(actor, volume);
	m_actors[index].add(actor);
	if(actors.isStatic(actor))
		m_staticVolume[index] += volume;
	else
		m_dynamicVolume[index] += volume;
}
void Blocks::actor_erase(BlockIndex index, ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	auto& blockActors = m_actors[index];
	auto& blockActorVolume = m_actorVolume[index];
	auto iter = std::ranges::find(blockActors, actor);
	assert(iter != blockActors.end());
	auto iter2 = m_actorVolume[index].begin() + (std::distance(blockActors.begin(), iter));
	assert(iter2 != blockActorVolume.end());
	if(actors.isStatic(actor))
		m_staticVolume[index] -= iter2->second;
	else
		m_dynamicVolume[index] -= iter2->second;
	blockActors.remove(iter);
	(*iter2) = blockActorVolume.back();
	blockActorVolume.pop_back();
}
void Blocks::actor_setTemperature(BlockIndex index, Temperature temperature)
{
	assert(temperature_get(index) == temperature);
	Actors& actors = m_area.getActors();
	for(auto pair : m_actorVolume[index])
		actors.temperature_onChange(pair.first);
}
void Blocks::actor_updateIndex(BlockIndex index, ActorIndex oldIndex, ActorIndex newIndex)
{
	auto found = std::ranges::find(m_actors[index], oldIndex);
	assert(found != m_actors[index].end());
	(*found) = newIndex; 
	auto found2 = std::ranges::find(m_actorVolume[index], oldIndex, &std::pair<ActorIndex, CollisionVolume>::first);
	assert(found2 != m_actorVolume[index].end());
	found2->first = newIndex; 
}
bool Blocks::actor_contains(BlockIndex index, ActorIndex actor) const
{
	return m_actors[index].contains(actor);
}
bool Blocks::actor_empty(BlockIndex index) const
{
	return m_actors[index].empty();
}
ActorIndicesForBlock& Blocks::actor_getAll(BlockIndex index)
{
	return m_actors[index];
}
