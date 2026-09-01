#include "space.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../simulation/hasItems.h"
#include "../numericTypes/types.h"
#include "../definitions/itemType.h"
#include <iterator>
void Space::item_record(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, ItemIndex item)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_recordStatic(mapWithCuboidKeys, item);
	else
		item_recordDynamic(mapWithCuboidKeys, item);
}
void Space::item_recordStatic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, ItemIndex item)
{
	Items& items = m_area.getItems();
	assert(items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.insert(pair.first, item);
	// Iterate twice for cache locality.
	for(const auto& pair : mapWithCuboidKeys)
		m_staticVolume.updateAdd(pair.first, pair.second);
}
void Space::item_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, ItemIndex item)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.insert(pair.first, item);
	for(const auto& pair : mapWithCuboidKeys)
		m_dynamicVolume.updateAdd(pair.first, pair.second);
}
void Space::item_erase(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, ItemIndex item)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_eraseStatic(mapWithCuboidKeys, item);
	else
		item_eraseDynamic(mapWithCuboidKeys, item);
}
void Space::item_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, ItemIndex item)
{
	Items& items = m_area.getItems();
	assert(!items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
	{
		m_items.remove(pair.first, item);
		m_dynamicVolume.updateSubtract(pair.first, pair.second);
	}
}
void Space::item_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& mapWithCuboidKeys, ItemIndex item)
{
	Items& items = m_area.getItems();
	assert(items.isStatic(item));
	for(const auto& pair : mapWithCuboidKeys)
		m_items.remove(pair.first, item);
	for(const auto& pair : mapWithCuboidKeys)
		m_staticVolume.updateSubtract(pair.first, pair.second);
}
void Space::item_disperseAll(Point3D point)
{
	auto& itemsInPoint = m_items.queryGetAll(point);
	if(itemsInPoint.empty())
		return;
	SmallSet<Point3D> points;
	for(Point3D otherIndex : getDirectlyAdjacentOnSameZLevelOnly(point))
		if(!solid_isAny(otherIndex))
			points.insert(otherIndex);
	auto copy = itemsInPoint;
	Items& items = m_area.getItems();
	for(auto item : copy)
	{
		//TODO: split up stacks of generics, prefer space with more empty space.
		Point3D newLocation = points[m_area.m_simulation.m_random.getInRange(0u, points.size() - 1u)];
		const Facing4 facing = (Facing4)(m_area.m_simulation.m_random.getInRange(0, 3));
		// TODO: use location_tryToSetStatic and find another location on fail.
		items.location_setStatic(item, newLocation, facing);
	}
}
void Space::item_updateIndex(Cuboid cuboid, ItemIndex oldIndex, ItemIndex newIndex)
{
	m_items.update(cuboid, oldIndex, newIndex);
}
void Space::item_addChunksAndPiles(const CuboidSet& cuboids, CollisionVolume volume, MaterialTypeId materialType)
{
	// Divide volume between chunks and piles.
	static ItemTypeId chunkType = ItemType::byName("chunk");
	static ItemTypeId pileType = ItemType::byName("pile");
	static ShapeId chunkShape{ItemType::getShape(chunkType)};
	static ShapeId pileShape{ItemType::getShape(pileType)};
	static CollisionVolume chunkVolume{Shape::getTotalCollisionVolume(chunkShape)};
	Quantity chunkQuantity{(volume / chunkVolume).get()};
	// Pile volume is always 1.
	assert(Shape::getTotalCollisionVolume(pileShape) == 1);
	Quantity pileQuantity{volume.get() % chunkVolume.get()};
	for(Cuboid cuboid : cuboids)
		for(Point3D point : cuboid)
		{
			if(chunkQuantity != 0)
				item_addGeneric(point, chunkType, materialType, chunkQuantity);
			if(pileQuantity != 0)
				item_addGeneric(point, pileType, materialType, pileQuantity);
		}
}
ItemIndex Space::item_addGeneric(Point3D point, const ItemTypeId itemType, const MaterialTypeId materialType, const Quantity quantity)
{
	// Add to an existing stack or create a new one.
	assert(shape_anythingCanEnterEver(point));
	assert(ItemType::getIsGeneric(itemType));
	Items& items = m_area.getItems();
	const auto condition = [itemType, &items](const auto& item) { return items.getItemType(item) == itemType; };
	ItemIndex existingStack = m_items.queryGetOneWithCondition(point, condition);
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
Quantity Space::item_getCount(Point3D point, const ItemTypeId itemType, const MaterialTypeId materialType) const
{
	// Returns 1 for nongenerics.
	Items& items = m_area.getItems();
	const auto condition = [&](ItemIndex item){ return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType; };
	ItemIndex item = m_items.queryGetOneWithCondition(point, condition);
	if(item.empty())
		return {0};
	return m_area.getItems().getQuantity(item);
}
ItemIndex Space::item_getGeneric(Point3D point, const ItemTypeId itemType, const MaterialTypeId materialType) const
{
	assert(ItemType::getIsGeneric(itemType));
	Items& items = m_area.getItems();
	const auto condition = [&](ItemIndex item){ return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType; };
	return m_items.queryGetOneWithCondition(point, condition);
}
bool Space::item_hasInstalledType(Point3D point, const ItemTypeId itemType) const
{
	assert(ItemType::getInstallable(itemType));
	Items& items = m_area.getItems();
	const auto condition = [&](ItemIndex item){ return items.getItemType(item) == itemType && items.isInstalled(item); };
	return m_items.queryAnyWithCondition(point, condition);
}
bool Space::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(Point3D point, ActorIndex actor) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	const auto condition = [&](ItemIndex item){
		const ItemTypeId itemType = items.getItemType(item);
		return(
			ItemType::getInternalVolume(itemType) != 0 &&
			ItemType::getCanHoldFluids(itemType) &&
			actors.canPickUp_anyWithMass(actor, items.getMass(item))
		);
	};
	return m_items.queryAnyWithCondition(point, condition);
}
bool Space::item_hasContainerContainingFluidTypeCarryableBy(Point3D point, ActorIndex actor, FluidTypeId fluidType) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	const auto condition = [&](ItemIndex item){
		const ItemTypeId itemType = items.getItemType(item);
		return(
			ItemType::getInternalVolume(itemType) != 0 &&
			items.cargo_getFluidType(item) == fluidType &&
			actors.canPickUp_anyWithMass(actor, items.getMass(item))
		);
	};
	return m_items.queryAnyWithCondition(point, condition);
}
bool Space::item_contains(Point3D point, ItemIndex item) const
{
	const auto condition = [&](ItemIndex i){ return i == item; };
	return m_items.queryAnyWithCondition(point, condition);
}
