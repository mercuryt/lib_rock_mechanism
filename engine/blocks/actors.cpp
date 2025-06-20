#include "blocks.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "numericTypes/types.h"
void Blocks::actor_recordDynamic(const BlockIndex& index, const ActorIndex& actor, const CollisionVolume& volume)
{
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	m_actorVolume[index].emplace_back(actor, volume);
	m_actors[index].insert(actor);
	m_dynamicVolume[index] += volume;
}
void Blocks::actor_recordStatic(const BlockIndex& index, const ActorIndex& actor, const CollisionVolume& volume)
{
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	m_actorVolume[index].emplace_back(actor, volume);
	m_actors[index].insert(actor);
	m_staticVolume[index] += volume;
}
void Blocks::actor_eraseStatic(const BlockIndex& index, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	auto& blockActors = m_actors[index];
	auto& blockActorVolume = m_actorVolume[index];
	auto iter = blockActors.find(actor);
	uint16_t offset = &*iter - &*blockActors.begin();
	auto iter2 = m_actorVolume[index].begin() + offset;
	m_staticVolume[index] -= iter2->second;
	blockActors.erase(iter);
	(*iter2) = blockActorVolume.back();
	blockActorVolume.pop_back();
}
void Blocks::actor_eraseDynamic(const BlockIndex& index, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	auto& blockActors = m_actors[index];
	auto& blockActorVolume = m_actorVolume[index];
	auto iter = blockActors.find(actor);
	uint16_t offset = &*iter - &*blockActors.begin();
	auto iter2 = m_actorVolume[index].begin() + offset;
	m_dynamicVolume[index] -= iter2->second;
	blockActors.erase(iter);
	(*iter2) = blockActorVolume.back();
	blockActorVolume.pop_back();
}
void Blocks::actor_setTemperature(const BlockIndex& index, [[maybe_unused]] const Temperature& temperature)
{
	assert(temperature_get(index) == temperature);
	Actors& actors = m_area.getActors();
	for(const ActorIndex& actor : m_actors[index])
		actors.temperature_onChange(actor);
}
void Blocks::actor_updateIndex(const BlockIndex& index, const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	auto found = m_actors[index].find(oldIndex);
	assert(found != m_actors[index].end());
	(*found) = newIndex;
	auto found2 = std::ranges::find(m_actorVolume[index], oldIndex, &std::pair<ActorIndex, CollisionVolume>::first);
	assert(found2 != m_actorVolume[index].end());
	found2->first = newIndex;
}
bool Blocks::actor_contains(const BlockIndex& index, const ActorIndex& actor) const
{
	return m_actors[index].contains(actor);
}
bool Blocks::actor_empty(const BlockIndex& index) const
{
	return m_actors[index].empty();
}
SmallSet<ActorIndex>& Blocks::actor_getAll(const BlockIndex& index)
{
	return m_actors[index];
}
const SmallSet<ActorIndex>& Blocks::actor_getAll(const BlockIndex& index) const
{
	return m_actors[index];
}
