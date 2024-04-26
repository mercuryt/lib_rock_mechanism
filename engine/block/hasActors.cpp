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
		m_block.m_area->m_hasActors.m_locationBuckets.update(actor, *actor.m_location, m_block);
		actor.m_location->m_hasActors.exit(actor);
	}
	else
		m_block.m_area->m_hasActors.m_locationBuckets.insert(actor, m_block);
	m_actors.push_back(&actor);
	m_block.m_hasShapes.enter(actor);
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
