#include "space.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../simulation/hasItems.h"
#include "../numericTypes/types.h"
#include "../definitions/itemType.h"
#include <iterator>
void Space::item_record(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_recordStatic(mapWithCuboidKeys, item);
	else
		item_recordDynamic(mapWithCuboidKeys, item);
}
void Space::item_recordStatic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.insert(pair.first, item);
	// Iterate twice for cache locality.
	for(const auto& pair : mapWithCuboidKeys)
		m_staticVolume.updateAddAll(pair.first, pair.second);
}
void Space::item_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.insert(pair.first, item);
	for(const auto& pair : mapWithCuboidKeys)
		m_dynamicVolume.updateAddAll(pair.first, pair.second);
}
void Space::item_erase(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_eraseStatic(mapWithCuboidKeys, item);
	else
		item_eraseDynamic(mapWithCuboidKeys, item);
}
void Space::item_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.remove(pair.first, item);
	for(const auto& pair : mapWithCuboidKeys)
		m_dynamicVolume.updateSubtractAll(pair.first, pair.second);
}
void Space::item_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.remove(pair.first, item);
	for(const auto& pair : mapWithCuboidKeys)
		m_staticVolume.updateSubtractAll(pair.first, pair.second);
}
void Space::item_setTemperature(const Point3D& point, const Temperature& temperature)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : m_items.queryGetAll(point))
		items.setTemperature(item, temperature, point);
}
void Space::item_disperseAll(const Point3D& point)
{
	auto& itemsInPoint = m_items.queryGetAll(point);
	if(itemsInPoint.empty())
		return;
	SmallSet<Point3D> points;
	for(Point3D otherIndex : getAdjacentOnSameZLevelOnly(point))
		if(!solid_isAny(otherIndex))
			points.insert(otherIndex);
	auto copy = itemsInPoint;
	Items& items = m_area.getItems();
	for(auto item : copy)
	{
		//TODO: split up stacks of generics, prefer space with more empty space.
		const Point3D newLocation = points[m_area.m_simulation.m_random.getInRange(0u, points.size() - 1u)];
		const Facing4 facing = (Facing4)(m_area.m_simulation.m_random.getInRange(0, 3));
		// TODO: use location_tryToSetStatic and find another location on fail.
		items.location_setStatic(item, newLocation, facing);
	}
}
void Space::item_updateIndex(const Cuboid& cuboid, const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	m_items.update(cuboid, oldIndex, newIndex);
}
ItemIndex Space::item_addGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	assert(shape_anythingCanEnterEver(point));
	assert(ItemType::getIsGeneric(itemType));
	Items& items = m_area.getItems();
	const auto condition = [itemType, &items](const auto& item) { return items.getItemType(item) == itemType; };
	const ItemIndex existingStack = m_items.queryGetOneWithCondition(point, condition);
	if(existingStack.exists())
	{
		items.addQuantity(existingStack, quantity);
		return existingStack;
	}
	return items.create(ItemParamaters{
		.itemType=itemType,
		.materialType=materialType,
		.location=point,
		.quantity=quantity,
	});
}
Quantity Space::item_getCount(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	Items& items = m_area.getItems();
	const auto condition = [&](const ItemIndex& item){ return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType; };
	const ItemIndex item = m_items.queryGetOneWithCondition(point, condition);
	if(item.empty())
		return {0};
	return m_area.getItems().getQuantity(item);
}
ItemIndex Space::item_getGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	assert(ItemType::getIsGeneric(itemType));
	Items& items = m_area.getItems();
	const auto condition = [&](const ItemIndex& item){ return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType; };
	return m_items.queryGetOneWithCondition(point, condition);
}
bool Space::item_hasInstalledType(const Point3D& point, const ItemTypeId& itemType) const
{
	assert(ItemType::getInstallable(itemType));
	Items& items = m_area.getItems();
	const auto condition = [&](const ItemIndex& item){ return items.getItemType(item) == itemType && items.isInstalled(item); };
	return m_items.queryAnyWithCondition(point, condition);
}
bool Space::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Point3D& point, const ActorIndex& actor) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	const auto condition = [&](const ItemIndex& item){
		const ItemTypeId itemType = items.getItemType(item);
		return(
			ItemType::getInternalVolume(itemType) != 0 &&
			ItemType::getCanHoldFluids(itemType) &&
			actors.canPickUp_anyWithMass(actor, items.getMass(item))
		);
	};
	return m_items.queryAnyWithCondition(point, condition);
}
bool Space::item_hasContainerContainingFluidTypeCarryableBy(const Point3D& point, const ActorIndex& actor, const FluidTypeId& fluidType) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	const auto condition = [&](const ItemIndex& item){
		const ItemTypeId itemType = items.getItemType(item);
		return(
			ItemType::getInternalVolume(itemType) != 0 &&
			items.cargo_getFluidType(item) == fluidType &&
			actors.canPickUp_anyWithMass(actor, items.getMass(item))
		);
	};
	return m_items.queryAnyWithCondition(point, condition);
}
bool Space::item_contains(const Point3D& point, const ItemIndex& item) const
{
	const auto condition = [&](const ItemIndex& i){ return i == item; };
	return m_items.queryAnyWithCondition(point, condition);
}
