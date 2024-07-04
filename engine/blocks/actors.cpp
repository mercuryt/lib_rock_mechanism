#include "blocks.h"
#include "../actors/actors.h"
#include "../area.h"
#include "types.h"
void Blocks::actor_record(BlockIndex index, ActorIndex actor, CollisionVolume volume)
{
	Actors& actors = m_area.getActors();
	m_actorVolume.at(index).emplace_back(actor, volume);
	m_actors.at(index).push_back(actor);
	if(actors.isStatic(actor))
		m_staticVolume.at(index) += volume;
	else
		m_dynamicVolume.at(index) += volume;
}
void Blocks::actor_erase(BlockIndex index, ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	auto& blockActors = m_actors.at(index);
	auto& blockActorVolume = m_actorVolume.at(index);
	auto iter = std::ranges::find(blockActors, actor);
	auto iter2 = m_actorVolume.at(index).begin() + (std::distance(iter, m_dynamicVolume.begin()));
	if(actors.isStatic(actor))
		m_staticVolume.at(index) -= iter2->second;
	else
		m_dynamicVolume.at(index) -= iter2->second;
	(*iter) = blockActors.back();
	(*iter2) = blockActorVolume.back();
	blockActorVolume.pop_back();
	blockActors.pop_back();
}
void Blocks::actor_setTemperature(BlockIndex index, Temperature temperature)
{
	assert(temperature_get(index) == temperature);
	Actors& actors = m_area.getActors();
	for(auto pair : m_actorVolume.at(index))
		actors.temperature_onChange(pair.first);
}
bool Blocks::actor_contains(BlockIndex index, ActorIndex actor) const
{
	return std::ranges::find(m_actorVolume.at(index), actor, &std::pair<ActorIndex, CollisionVolume>::first) != m_actorVolume.at(index).end();
}
bool Blocks::actor_empty(BlockIndex index) const
{
	return m_actorVolume.at(index).empty();
}
std::vector<ActorIndex>& Blocks::actor_getAll(BlockIndex index)
{
	return m_actors.at(index);
}
