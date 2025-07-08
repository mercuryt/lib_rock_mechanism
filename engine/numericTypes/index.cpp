#include "numericTypes/index.h"
#include "actorOrItemIndex.h"
HasShapeIndex::HasShapeIndex(const PlantIndex& index) { data = index.get(); }
HasShapeIndex::HasShapeIndex(const ItemIndex& index) { data = index.get(); }
HasShapeIndex::HasShapeIndex(const ActorIndex& index) { data = index.get(); }
ActorOrItemIndex ActorIndex::toActorOrItemIndex() const
{
	return ActorOrItemIndex::createForActor(ActorIndex::create(data));
}
ActorOrItemIndex ItemIndex::toActorOrItemIndex() const
{
	return ActorOrItemIndex::createForItem(ItemIndex::create(data));
}