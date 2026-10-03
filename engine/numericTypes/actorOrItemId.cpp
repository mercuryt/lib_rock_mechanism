#include "actorOrItemId.h"
#include "../simulation/simulation.h"
#include "../simulation/hasActors.h"
ActorOrItemIndex ActorOrItemId::getIndex(const Simulation& simulation) const
{
	return m_isActor ?
		ActorOrItemIndex::create(simulation.m_actors.getIndexForId(getActor())) :
		ActorOrItemIndex::create(simulation.m_items.getIndexForId(getItem()));
}