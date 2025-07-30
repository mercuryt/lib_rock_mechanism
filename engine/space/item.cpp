#include "space.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../simulation/hasItems.h"
#include "../numericTypes/types.h"
#include "../definitions/itemType.h"
#include <iterator>
void Space::item_record(const Point3D& point, const ItemIndex& item, const CollisionVolume& volume)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_recordStatic(point, item, volume);
	else
		item_recordDynamic(point, item, volume);
}
void Space::item_recordStatic(const Point3D& point, const ItemIndex& item, const CollisionVolume& volume)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	m_items.updateOrInsertOne(point, [&](SmallSet<ItemIndex>& items){ items.insert(item); });
	m_itemVolume.updateOrInsertOne(point, [&](SmallMap<ItemIndex, CollisionVolume>& itemVolumes){ itemVolumes.insert(item, volume); });
	m_staticVolume.updateAddOne(point, volume);
}
void Space::item_recordDynamic(const Point3D& point, const ItemIndex& item, const CollisionVolume& volume)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	m_items.updateOrInsertOne(point, [&](SmallSet<ItemIndex>& items){ items.insert(item); });
	m_itemVolume.updateOrInsertOne(point, [&](SmallMap<ItemIndex, CollisionVolume>& itemVolumes){ itemVolumes.insert(item, volume); });
	m_dynamicVolume.updateAddOne(point, volume);
}
void Space::item_erase(const Point3D& point, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_eraseStatic(point, item);
	else
		item_eraseDynamic(point, item);
}
void Space::item_eraseDynamic(const Point3D& point, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	m_items.updateOne(point, [&](SmallSet<ItemIndex>& items){ items.erase(item); });
	CollisionVolume volume;
	m_itemVolume.updateOne(point, [&](SmallMap<ItemIndex, CollisionVolume>& itemVolumes){
		volume = itemVolumes[item];
		itemVolumes.erase(item);
	});
	m_staticVolume.updateSubtractOne(point, volume);
}
void Space::item_eraseStatic(const Point3D& point, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	m_items.updateOne(point, [&](SmallSet<ItemIndex>& items){ items.erase(item); });
	CollisionVolume volume;
	m_itemVolume.updateOne(point, [&](SmallMap<ItemIndex, CollisionVolume>& itemVolumes){
		volume = itemVolumes[item];
		itemVolumes.erase(item);
	});
	m_staticVolume.updateSubtractOne(point, volume);
}
void Space::item_setTemperature(const Point3D& point, const Temperature& temperature)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : m_items.queryGetOne(point))
		items.setTemperature(item, temperature, point);
}
void Space::item_disperseAll(const Point3D& point)
{
	auto& itemsInPoint = m_items.queryGetOne(point);
	if(itemsInPoint.empty())
		return;
	SmallSet<Point3D> points;
	for(Point3D otherIndex : getAdjacentOnSameZLevelOnly(point))
		if(!solid_is(otherIndex))
			points.insert(otherIndex);
	auto copy = itemsInPoint;
	Items& items = m_area.getItems();
	for(auto item : copy)
	{
		//TODO: split up stacks of generics, prefer space with more empty space.
		const Point3D point = points[m_area.m_simulation.m_random.getInRange(0u, points.size() - 1u)];
		const Facing4 facing = (Facing4)(m_area.m_simulation.m_random.getInRange(0, 3));
		// TODO: use location_tryToSetStatic and find another location on fail.
		items.location_setStatic(item, point, facing);
	}
}
void Space::item_updateIndex(const Point3D& point, const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	m_items.updateOne(point, [&](SmallSet<ItemIndex>& data){ data.update(oldIndex, newIndex); });
	m_itemVolume.updateOne(point, [&](SmallMap<ItemIndex, CollisionVolume>& data){ data.updateKey(oldIndex, newIndex); });
}
ItemIndex Space::item_addGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	assert(shape_anythingCanEnterEver(point));
	Items& items = m_area.getItems();
	CollisionVolume volume = Shape::getCollisionVolumeAtLocation(ItemType::getShape(itemType)) * quantity;
	// Assume that generics added through this method are static.
	// Generics in transit ( being hauled by multiple workers ) should not use this method and should use setLocationAndFacing instead.
	m_staticVolume.updateAddOne(point, volume);
	const auto& pointItems = m_items.queryGetOne(point);
	auto found = pointItems.findIf([itemType, &items](const auto& item) { return items.getItemType(item) == itemType; });
	if(found == pointItems.end())
	{
		ItemIndex item = items.create(ItemParamaters{
			.itemType=itemType,
			.materialType=materialType,
			.location=point,
			.quantity=quantity,
		});
		return item;
	}
	else
	{
		ItemIndex item = *found;
		assert(item.exists());
		items.addQuantity(item, quantity);
		return item;
	}
}
Quantity Space::item_getCount(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	auto& itemsInPoint = m_items.queryGetOne(point);
	Items& items = m_area.getItems();
	const auto found = itemsInPoint.findIf([&](const ItemIndex& item)
	{
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInPoint.end())
		return Quantity::create(0);
	else
		return items.getQuantity(*found);
}
ItemIndex Space::item_getGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	auto& itemsInPoint = m_items.queryGetOne(point);
	Items& items = m_area.getItems();
	const auto found = itemsInPoint.findIf([&](const ItemIndex& item) {
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInPoint.end())
		return ItemIndex::null();
	return *found;
}
const SmallSet<ItemIndex> Space::item_getAll(const Point3D& point) const
{
	SmallSet<ItemIndex> output;
	for(const ItemIndex& item : m_items.queryGetOne(point))
		output.insert(item);
	return output;
}
// TODO: buggy
bool Space::item_hasInstalledType(const Point3D& point, const ItemTypeId& itemType) const
{
	const auto& itemsInPoint = m_itemVolume.queryGetOne(point);
	Items& items = m_area.getItems();
	for(const auto& [item, volume] : itemsInPoint)
		if(items.getItemType(item) == itemType)
			return items.isInstalled(item);
	return false;
}
bool Space::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Point3D& point, const ActorIndex& actor) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(const ItemIndex& item : m_items.queryGetOne(point))
	{
		ItemTypeId itemType = items.getItemType(item);
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(ItemType::getInternalVolume(itemType) != 0 && ItemType::getCanHoldFluids(itemType) && actors.canPickUp_anyWithMass(actor, items.getMass(item)))
			return true;
	}
	return false;
}
bool Space::item_hasContainerContainingFluidTypeCarryableBy(const Point3D& point, const ActorIndex& actor, const FluidTypeId& fluidType) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(const ItemIndex& item : m_items.queryGetOne(point))
	{
		ItemTypeId itemType = items.getItemType(item);
		if(ItemType::getInternalVolume(itemType) != 0 && ItemType::getCanHoldFluids(itemType) &&
			actors.canPickUp_anyWithMass(actor, items.getMass(item)) &&
			items.cargo_getFluidType(item) == fluidType
		)
			return true;
	}
	return false;
}
bool Space::item_empty(const Point3D& point) const
{
	return !m_itemVolume.queryAny(point);
}
bool Space::item_contains(const Point3D& point, const ItemIndex& item) const
{
	return m_items.queryGetOne(point).contains(item);
}
