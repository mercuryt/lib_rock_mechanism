#include "reference.h"
#include "actors/actors.h"
#include "area.h"
#include "items/items.h"
ActorReference::ActorReference(Area& area, ActorIndex index) { m_target = &area.getActors().getReferenceTarget(index); }
ItemReference::ItemReference(Area& area, const ItemIndex& index) { m_target = &area.getItems().getReferenceTarget(index); }
void ActorReference::load(const Json& data, Area& area) { m_target = &area.getActors().getReferenceTarget(data.get<ActorIndex>()); }
void ItemReference::load(const Json& data, Area& area) { m_target = &area.getItems().getReferenceTarget(data.get<ItemIndex>()); }
void ActorOrItemReference::load(const Json& data, Area& area)
{
	ActorOrItemIndex index;
	nlohmann::from_json(data, index);
	if(index.isActor())
		setActor(area.getActors().getReferenceTarget(index.getActor()));
	else
		setItem(area.getItems().getReferenceTarget(index.getItem()));
}
void ItemReferences::addGeneric(Area& area, ItemTypeId itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	auto found = find([&](const ItemReference& item) {
		Items& items = area.getItems();
		ItemIndex itemIndex = item.getIndex();
		return items.getItemType(itemIndex) == itemType && items.getMaterialType(itemIndex) == materialType;
	});
	if(found == data.end())
	{
		ItemIndex equipment = area.getItems().create({
			.itemType = itemType,
			.materialType = materialType,
			.quantity = quantity,
		});
		add(equipment, area);
	}
	else
	{
		ItemIndex equipment = found->getIndex();
		area.getItems().addQuantity(equipment, quantity);
	}
}
void ItemReferences::removeGeneric(Area& area, ItemTypeId itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	Items& items = area.getItems();
	auto found = find([&](const ItemReference& item) {
		ItemIndex itemIndex = item.getIndex();
		return items.getItemType(itemIndex) == itemType && items.getMaterialType(itemIndex) == materialType;
	});
	assert(found != data.end());
	if(quantity == items.getQuantity(found->getIndex()))
		remove(*found);
	else
		items.removeQuantity(found->getIndex(), quantity);
}
