#include "blocks.h"
#include "../actor.h"
#include "../area.h"
#include "types.h"
void Blocks::actor_enter(BlockIndex index, Actor& actor)
{
	assert(!actor_contains(index, actor));
	std::unordered_set<BlockIndex> oldBlocks = actor.m_blocks;
	if(actor.m_location != BLOCK_INDEX_MAX)
	{
		actor.m_facing = facingToSetWhenEnteringFrom(index, actor.m_location);
		actor_exit(actor.m_location, actor);
	}
	m_actors.at(index).push_back(&actor);
	shape_enter(index, actor);
	if(oldBlocks.empty())
		m_area.m_hasActors.m_locationBuckets.add(actor);
	else
		m_area.m_hasActors.m_locationBuckets.update(actor, oldBlocks);
	if(m_underground[index])
		m_area.m_hasActors.setUnderground(actor);
	else
		m_area.m_hasActors.setNotUnderground(actor);
}
void Blocks::actor_exit(BlockIndex index, Actor& actor)
{
	assert(actor_contains(index, actor));
	std::erase(m_actors.at(index), &actor);
	shape_exit(index, actor);
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
