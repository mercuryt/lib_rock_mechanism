#include "index.h"
#include "actors/actors.h"
#include "area.h"
#include "items/items.h"
#include "simulation.h"
#include "reference.h"
#include "actorOrItemIndex.h"
template<> BlockIndex BlockIndices::random(Simulation& simulation) const
{
	return simulation.m_random.getInVector(data);
}
ActorOrItemIndex ActorIndex::toActorOrItemIndex() const
{
	return ActorOrItemIndex::createForActor(data);
}
ActorOrItemIndex ItemIndex::toActorOrItemIndex() const
{
	return ActorOrItemIndex::createForItem(data);
}
ItemReference ItemIndex::toReference(Area& area) const
{
	return area.getItems().getReference(data);
}
ActorReference ActorIndex::toReference(Area& area) const
{
	return area.getActors().getReference(data);
}
