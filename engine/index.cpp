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
HasShapeIndex::HasShapeIndex(const PlantIndex& index) { data = index.get(); }
HasShapeIndex::HasShapeIndex(const ItemIndex& index) { data = index.get(); }
HasShapeIndex::HasShapeIndex(const ActorIndex& index) { data = index.get(); }
ActorOrItemIndex ActorIndex::toActorOrItemIndex() const
{
	return ActorOrItemIndex::createForActor(data);
}
ActorOrItemIndex ItemIndex::toActorOrItemIndex() const
{
	return ActorOrItemIndex::createForItem(data);
}