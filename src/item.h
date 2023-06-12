#pragma once

#include "body.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <unordered_multimap>


template<class WoundType>
struct AttackType
{
	std::string name;
	uint32_t area;
	uint32_t baseForce;
	const WoundType& woundType;
};
class WeaponType
{
	const std::vector<AttackType> attackTypes;
};
class WearableType
{
	const uint32_t percentCoverage;
	const std::unordered_set<BodyPartType*> bodyPartsCovered;
	const uint32_t defenseScore;
	const bool rigid;
	const uint32_t impactSpreadArea;
	const uint32_t forceAbsorbedPiercedModifier;
	const uint32_t forceAbsorbedUnpiercedModifier;

};
class ItemType
{
	const std::string name;
	const MaterialType& materialType;
	const uint32_t quality;
	const uint32_t wear;
	const WeaponType& weaponType;
       	const WearableType& wearableType;
	const GenericType& genericType;
};
template<class ItemType, class MaterialType>
struct Item
{
	const uint32_t id;
	const ItemType& itemType;
	const MaterialType& materialType;
	uint32_t quantity; // Only used by generic item types.
	std::string name;
	uint32_t quality;
	Reservable reservable;
	uint32_t mass;
	uint32_t percentWear;

	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q):
		id(i), itemType(it), materialType(mt), quantity(q), reservable(q)
	{
		assert(itemType.generic);
		mass = itemType.volume * materialType.mass * quantity;
	}
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, uint32_t pw):
		id(i), itemType(it), materialType(mt), quantity(1), name(n), quality(qual), reservable(1), percentWear(pw)
	{
		assert(!itemType.generic);
		mass = itemType.volume * materialType.mass * quantity;
	}
	static uint32_t s_nextId;
	struct Hash{ std::size_t operator()(const Item& item) const { return item.id; }};
	static std::unordered_set<Item, Hash> s_globalItems;
	// Generic items, created in local item set.
	static Item& create(Area& area, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
	{
		assert(itemType.generic);
		area.m_items.emplace(++s_nextId, itemType, materialType, quantity);
	}
	static Item& create(Area& area, const uint32_t id,  const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
	{
		assert(itemType.generic);
		if(!s_nextId > id) s_nextId = id + 1;
		area.m_items.emplace(id, itemType, materialType, quantity);
	}
	// Unnamed items, created in local item set.
	static Item& create(Area& area, const ItemType& itemType, const MaterialType& materialType, uint32_t quality, uint32_t percentWear)
	{
		assert(!itemType.generic);
		area.m_items.emplace(++s_nextId, itemType, materialType, 1, "", quality, percentWear);
	}
	static Item& create(Area& area, const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quality, uint32_t percentWear)
	{
		assert(!itemType.generic);
		if(!s_nextId > id) s_nextId = id + 1;
		area.m_items.emplace(id, itemType, materialType, 1, "", quality, percentWear);
	}
	// Named items, created in global item set.
	static Item& create(const ItemType& itemType, const MaterialType& materialType, std::string name, uint32_t quality, uint32_t percentWear)
	{
		assert(!itemType.generic);
		assert(!name.empty());
		s_globalItems.emplace(++s_nextId, itemType, materialType, 1, name, quality, percentWear);
	}
	static Item& create(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, std::string name, uint32_t quality, uint32_t percentWear)
	{
		assert(!itemType.generic);
		assert(!name.empty());
		if(!s_nextId > id) s_nextId = id + 1;
		s_globalItems.emplace(id, itemType, materialType, 1, name, quality, percentWear);
	}
	//TODO: moveTo
};
Item::s_nextId = 0;
// For use by block and actor.
template<class Item, class ItemType>
class HasItems
{
	std::unordered_map<const ItemType*, std::unordered_set<Item*>> m_items;
	uint32_t m_mass;
	HasItems() : m_mass(0) { }
	void add(Item& item)
	{
		m_mass += item.mass;
		m_item[&item.itemType].insert(&item);
	}
	void remove(Item& item)
	{
		assert(m_items.contains(&item.itemType));
		m_items[&item.itemType].remove(&item);
		m_mass -= item.mass;
	}
	void add(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
	{
		assert(itemType.generic);
		if(m_items.contains(&itemType))
			for(Item& i : m_items.at(&itemType))
				if(i.materialType == &materialType)
				{
					i.quantity += quantity;
					i->reservable.setMaxReservations(quantity);
					m_mass += Item::getMass(itemType, materialType) * quantity;
					return;
				}
		addItem(*Item::create(itemType, materialType, quantity));
	}
	void remove(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
	{
		assert(itemType.generic);
		assert(m_items.contains(&itemType));
		for(Item* i : m_items.at(&itemType))
			if(i->materialType == &materialType)
				if(i->quantity == quantity)
				{
					remove(*i);
					return;
				}
				else
				{
					i->quantity -= quantity;
					i->reservable.setMaxReservations(quantity);
					m_mass -= Item::getMass(itemType, materialType) * quantity;
					return;
				}
	}
	void tranferTo(HasItems& other, Item& item)
	{
		assert(!item.itemType.generic);
		other.add(&item);
		remove(&item);
	}
	void transferTo(HasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
	{
		assert(item.itemType.generic);
		assert(m_genericItems.contains(&itemType));
		assert(m_genericItems.at(&itemType) >= quantity);
		other.add(itemType, materialType, quantity);
		remove(itemType, materialType, quantity
	}
	bool has(Item& item)
	{
		if(!m_items.contains(&item.itemType))
			return false;
		return m_items.contains(*item);
	}
};
template<class Item, class Actor>
class CanPickup
{
	Actor& m_actor;
	HasItems m_hasItems;
	void pickUp(Item& item, uint8_t quantity)
	{
		assert(quantity <= item.quantity);
		m_actor.m_location->m_hasItems.tranferTo(m_hasItems, item, quantity)
		item.m_reservable.unreserve(m_actor.canReserve);
		m_actor.setCarryMass();
	}
	void putDown()
	{
		m_hasItems.transferTo(m_actor.m_location->m_hasItems, *m_carrying, m_carrying->quantity);
		m_carrying = nullptr;
		m_actor.setCarryMass();
	}
	uint32_t canPickupQuantityOf(const ItemType& itemType)
	{
		return (m_actor.maxCarryMass - m_actor.m_totalCarryMass) / itemType.mass;
	}
};
