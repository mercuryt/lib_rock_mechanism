#include "hasActors.h"
#include "../actor.h"
#include "../block.h"
#include "../area.h"
void BlockHasActors::enter(Actor& actor)
{
	assert(!contains(actor));
	if(actor.m_location != nullptr)
	{
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_location);
		actor.m_location->m_hasActors.exit(actor);
	}
	m_actors.push_back(&actor);
	std::unordered_set<Block*> oldBlocks = actor.m_blocks;
	m_block.m_hasShapes.enter(actor);
	if(oldBlocks.empty())
		m_block.m_area->m_hasActors.m_locationBuckets.add(actor);
	else
		m_block.m_area->m_hasActors.m_locationBuckets.update(actor, oldBlocks);
	if(m_block.m_underground)
		m_block.m_area->m_hasActors.setUnderground(actor);
	else
		m_block.m_area->m_hasActors.setNotUnderground(actor);
}
void BlockHasActors::exit(Actor& actor)
{
	assert(contains(actor));
	std::erase(m_actors, &actor);
	m_block.m_hasShapes.exit(actor);
}
void BlockHasActors::setTemperature(Temperature temperature)
{
	(void)temperature;
	for(Actor* actor : m_actors)
		actor->m_needsSafeTemperature.onChange();
}
bool BlockHasActors::contains(Actor& actor) const
{
	return std::ranges::find(m_actors, &actor) != m_actors.end();
}
