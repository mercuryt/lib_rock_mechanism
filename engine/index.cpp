#include "index.h"
#include "actors/actors.h"
#include "area/area.h"
#include "items/items.h"
#include "simulation/simulation.h"
#include "reference.h"
#include "actorOrItemIndex.h"
BlockIndex BlockIndex::dbg(const BlockIndexWidth& value) { return create(value); }
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