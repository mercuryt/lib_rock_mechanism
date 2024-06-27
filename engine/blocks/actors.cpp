#include "blocks.h"
#include "../actor.h"
#include "../area.h"
#include "types.h"
void Blocks::actor_record(BlockIndex index, ActorIndex actor, CollisionVolume volume)
{
	m_actors.at(index).emplace_back(actor, volume);
	if(m_area.m_actors.isStatic(actor))
		m_staticVolume.at(index) += volume;
	else
		m_dynamicVolume.at(index) += volume;
}
void Blocks::actor_erase(BlockIndex index, ActorIndex actor)
{
	assert(m_area.m_actors.getLocation(actor) == index);
	auto& actors = m_actors.at(index);
	auto found = std::ranges::find(actors, actor, &std::pair<ActorIndex, CollisionVolume>::first);
	assert(found != actors.end());
	if(m_area.m_actors.isStatic(actor))
		m_staticVolume -= found->second;
	else
		m_dynamicVolume -= found->second;
	actors.erase(found);
}
void Blocks::actor_setTemperature(BlockIndex index, Temperature temperature)
{
	assert(temperature_get(index) == temperature);
	for(Actor* actor : m_actors.at(index))
		actor->m_needsSafeTemperature.onChange();
}
bool Blocks::actor_contains(BlockIndex index, Actor& actor) const
{
	return std::ranges::find(m_actors.at(index), &actor) != m_actors.at(index).end();
}
bool Blocks::actor_empty(BlockIndex index) const
{
	return m_actors.at(index).empty();
}
std::vector<Actor*>& Blocks::actor_getAll(BlockIndex index)
{
	return m_actors[index];
}
