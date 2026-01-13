#include "numericTypes/index.h"
#include "actorOrItemIndex.h"
void to_json(Json& data, const VisionFacadeIndex& index) { data = index.get(); }
void from_json(const Json& data, VisionFacadeIndex& index) { index = VisionFacadeIndex::create(data.get<VisionFacadeIndexWidth>()); }
void to_json(Json& data, const PathRequestIndex& index) { data = index.get(); }
void from_json(const Json& data, PathRequestIndex& index) { index = PathRequestIndex::create(data.get<PathRequestIndexWidth>()); }
void to_json(Json& data, const HasShapeIndex& index) { data = index.get(); }
void from_json(const Json& data, HasShapeIndex& index) { index = HasShapeIndex::create(data.get<int>()); }
void to_json(Json& data, const PlantIndex& index) { data = index.get(); }
void from_json(const Json& data, PlantIndex& index) { index = PlantIndex::create(data.get<int>()); }
void to_json(Json& data, const ItemIndex& index) { data = index.get(); }
void from_json(const Json& data, ItemIndex& index) { index = ItemIndex::create(data.get<int>()); }
void to_json(Json& data, const ActorIndex& index) { data = index.get(); }
void from_json(const Json& data, ActorIndex& index) { index = ActorIndex::create(data.get<ActorIndexWidth>()); }
void to_json(Json& data, const AdjacentIndex& index) { data = index.get(); }
void from_json(const Json& data, AdjacentIndex& index) { index = AdjacentIndex::create(data.get<AdjacentIndexWidth>()); }
void to_json(Json& data, const ActorReferenceIndex& index) { data = index.get(); }
void from_json(const Json& data, ActorReferenceIndex& index) { index = ActorReferenceIndex::create(data.get<ActorReferenceIndexWidth>()); }
void to_json(Json& data, const ItemReferenceIndex& index) { data = index.get(); }
void from_json(const Json& data, ItemReferenceIndex& index) { index = ItemReferenceIndex::create(data.get<ItemReferenceIndexWidth>()); }
void to_json(Json& data, const RTreeNodeIndex& index) { data = index.get(); }
void from_json(const Json& data, RTreeNodeIndex& index) { index = RTreeNodeIndex::create(data.get<RTreeNodeIndexWidth>()); }
void to_json(Json& data, const RTreeArrayIndex& index) { data = index.get(); }
void from_json(const Json& data, RTreeArrayIndex& index) { index = RTreeArrayIndex::create(data.get<RTreeArrayIndexWidth>()); }
void to_json(Json& data, const SquadIndex& index) { data = index.get(); }
void from_json(const Json& data, SquadIndex& index) { index = SquadIndex::create(data.get<int>()); }
void to_json(Json& data, const SquadFormationIndex& index) { data = index.get(); }
void from_json(const Json& data, SquadFormationIndex& index) { index = SquadFormationIndex::create(data.get<int>()); }
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