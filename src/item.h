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
struct WeaponType
{
	const std::vector<AttackType> attackTypes;
};
struct WearableType
{
	const uint32_t percentCoverage;
	const std::unordered_set<BodyPartType*> bodyPartsCovered;
	const uint32_t defenseScore;
	const bool rigid;
	const uint32_t impactSpreadArea;
	const uint32_t forceAbsorbedPiercedModifier;
	const uint32_t forceAbsorbedUnpiercedModifier;

};
struct ItemType
{
	const std::string name;
	const bool installable;
	const Shape& shape;
	const bool generic;
	const WeaponType& weaponType;
       	const WearableType& wearableType;
	const uint32_t internalVolume;
	const bool canHoldFluids;
};
class ItemContainsItemsOrFluids
{
	Item& m_item;
	uint32_t m_volume;
	uint32_t m_mass;
	std::unordered_map<Item*, uint32_t> m_itemsAndVolumes;
	const FluidType* m_fluidType;
public:
	ItemContainsItemsOrFluids(Item* i) : m_item(i), m_volume(0), m_mass(0), m_fluidType(nullptr) { }
	void add(Item& item)
	{
		assert(!m_itemsAndVolumes.contains(&item));
		assert(canAdd(item));
		m_volume += item.getVolume();
		m_mass += item.getMass();
		assert(m_volume <= m_item.itemType.internalVolume);
		bool combined = false;
		if(item.itemType.generic)
			for(auto& pair : m_itemsAndVolumes)
				if(pair.first()->genericsCanCombine(item))
				{
					pair.first.quantity += item.quantity;
					item.destroy();
					combined = true;
				}
		if(!combined)
			m_itemsAndVolumes[&item] = item.getVolume();
	}
	void add(const FluidType& fluidType, uint32_t volume)
	{
		assert(canAdd(fluidType));
		m_fluidType = fluidType;
		m_volume += volume;
		m_mass += volume * fluidType.density;
	}
	void remove(const FluidType& fluidType, uint32_t volume)
	{
		assert(m_fluidType == fluidType);
		m_volume -= volume;
		m_mass -= volume * fluidType.density;
		if(m_volume == 0)
			m_fluidType = nullptr;
	}
	void remove(Item& item)
	{
		assert(m_itemsAndVolumes.contains(item));
		assert(m_volume >= item.getVolume());
		m_volume -= item.getVolume();
		m_mass -= item.getMass();
		m_itemsAndVolumes.remove(&item);
	}
	void remove(Item& item, uint32_t quantity)
	{
		assert(item.itemType.generic);
		assert(item.quantity >= quantity);
		item.quantity -= quantity;
		if(item.quantity == 0)
			m_itemsAndVolumes.remove(&item);
	}
	bool canAdd(Item& item) const
	{
		assert(&item != m_item);
		return m_fluidType == nullptr && m_volume + m_item.getVolume() <= m_item.itemType.internalVolume;
	}
	bool canAdd(FluidType& fluidType) const
	{
		return (m_fluidType == nullptr || m_fluidType == fluidType) && m_item.itemType.canHoldFluids && m_itemsAndVolumes.empty() && m_volume <= m_item.itemType.internalVolume;
	}
};
template<class ItemType, class MaterialType>
struct Item
{
	const uint32_t id;
	const ItemType& itemType;
	const MaterialType& materialType;
	uint32_t quantity; // Only used by generic item types.
	Reservable reservable;
	std::string name;
	uint32_t quality;
	uint32_t percentWear;
	bool installed;

	uint32_t getVolume() const { return quantity * itemType.volume; }
	uint32_t getMass() const 
	{ 
		uint32_t output = quantity * itemType.mass;
		if(itemType.internalVolume)
			for(Item* item : m_containsItemsOrFluids.getItems())
				output += item.getMass();
		return output;
       	}
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
template<class Item, class ItemType, class MaterialType, class MaterialTypeCategory>
class ItemQuery
{
	Item* m_item;
	const ItemType* m_itemType;
	const MaterialTypeCategory* m_materialTypeCategory;
	const MaterialType* m_materialType;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(Item& item) : m_item(&item), m_itemType(nullptr), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
	ItemQuery(const ItemType& itemType) : m_item(nullptr), m_itemType(&itemType), MaterialTypeCategory(nullptr), m_materialType(nullptr) { }
	ItemQuery(const ItemType& itemType, const MaterialTypeCategory& mtc) : m_item(nullptr), m_itemType(&itemType), m_materialTypeCategory(mtc), m_materialType(nullptr) { }
	ItemQuery(const ItemType& itemType, const MaterialType& mt) : m_item(nullptr), m_itemType(&itemType), m_materialTypeCategory(nullptr), m_materialType(mt) { }
	bool operator()(Item& item) const
	{
		if(const m_item != nullptr)
			return item == m_item;
		if(m_itemType != item.itemType)
			return false;
		if(m_materialTypeCategory != nullptr)
			return m_materialTypeCategory == item.materialType.materialTypeCategory;
		if(m_materialType != nullptr)
			return m_materialType == item.materialType;
		return true;
	}
	void specalize(Item& item)
	{
		assert(m_itemType != nullptr && m_item == nullptr && item.itemType == m_itemType);
		m_item = item;
		m_itemType = nullptr;
	}
	void specalize(MaterialType& materialType)
	{
		assert(m_materialTypeCategory == nullptr || materialType.materialTypeCategory == m_materialTypeCategory);
		assert(m_materialType == nullptr);
		m_materialType = materialType;
		m_materialTypeCategory = nullptr;
	}
};
// To be used by actor and vehicle.
template<class Item, class ItemType>
class HasItems
{
	std::vector<Item*> m_items;
	uint32_t m_mass;
	HasItems(): m_mass(0) { }
	// Non generic types have Shape.
	void add(Item& item)
	{
		assert(!item.itemType.generic);
		assert(std::ranges::find(m_items, &item) == m_items.end());
		m_mass += item.itemType.volume * item.materialType.mass;
		m_items.push_back(&item);
	}
	void remove(Item& item)
	{
		assert(!item.itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		m_mass -= item.itemType.volume * item.materialType.mass;
		std::erase(m_items, &item);
	}
	void add(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
	{
		assert(itemType.generic);
		m_mass += item.itemType.volume * item.materialType.mass * quantity;
		auto found = std::find(m_items, [&](Item* item) { return item.itemType == itemType && item.materialType == materialType; });
		// Add to stack.
		if(found != m_items.end())
		{
			found->quantity += quantity;
			found->reservable.setMaxReservations(found->quantity);
		}
		// Create.
		else
		{
			Item& item = Item::create(itemType, materialType, quantity);
			m_items.push_back(&item);
		}
	}
	void remove(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
	{
		assert(itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		m_mass -= item.itemType.volume * item.materialType.mass * quantity;
		auto found = std::find(m_items, [&](Item* item) { return item.itemType == itemType && item.materialType == materialType; });
		assert(found != m_items.end());
		assert(found->quantity >= quantity);
		// Remove all.
		if(found->quantity == quantity)
			m_items.erase(found);
		// Remove some.
		else
		{
			found->quantity -= quantity;
			found->reservable.setMaxReservations(found->quantity);
		}
	}
	void tranferTo(BlockHasItems& other, Item& item)
	{
		other.add(item);
		remove(item);
	}
	void transferTo(BlockHasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
	{
		other.add(itemType, materialType, quantity);
		remove(itemType, materialType, quantity
	}
	void tranferTo(HasItems& other, Item& item)
	{
		other.add(item);
		remove(item);
	}
	void transferTo(HasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
	{
		other.add(itemType, materialType, quantity);
		remove(itemType, materialType, quantity);
	}
	Item* get(ItemType& itemType) const
	{
		auto found = std::ranges::find(m_items, itemType, &Item::ItemType);
		if(found == m_items.end())
			return nullptr;
		return &*found;
	}
};
template<class Block, class Item, class ItemType>
class BlockHasItems
{
	Block& m_block;
	std::vector<Item*> m_items;
	uint32_t m_volume;
	BlockHasItems(Block& b): m_volume(0) , m_block(b) { }
	// Non generic types have Shape.
	void add(Item& item)
	{
		assert(!item.itemType.generic);
		assert(std::ranges::find(m_items, &item) == m_items.end());
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : item.itemType.shape.positions)
		{
			bool impassible = block->impassible();
			Block* block = offset(m_x, m_y, m_z);
			assert(block != nullptr);
			assert(!block->isSolid());
			assert(std::ranges::find(block->m_items, &item) == m_items.end());
			block->m_blockHasItems.volume += v;
			block->m_blockHasItems.m_items.push_back(&item);
			// Invalidate move cache when impassably full.
			if(!impassible && block->impassible())
				block->m_hasActors.invalidateCache();
		}
	}
	void remove(Item& item)
	{
		assert(!item.itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : item.itemType.shape.positions)
		{
			Block* block = offset(m_x, m_y, m_z);
			assert(block != nullptr);
			assert(!block->isSolid());
			assert(std::ranges::find(block->m_items, &item) != m_items.end());
			bool impassible = block->impassible();
			block->m_blockHasItems.m_volume -= v;
			std::erase(block->m_blockHasItem.m_items, &item);
			// Invalidate move cache when no-longer impassably full.
			if(impassible && !block->impassible())
				block->m_hasActors.invalidateCache();
		}
	}
	void add(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
	{
		assert(itemType.generic);
		auto found = std::find(m_items, [&](Item* item) { return item.itemType == itemType && item.materialType == materialType; });
		bool impassible = block->impassible();
		m_volume += itemType.volume * quantity;
		// Add to.
		if(found != m_items.end())
		{
			found->quantity += quantity;
			found->reservable.setMaxReservations(found->quantity);
		}
		// Create.
		else
		{
			Item& item = Item::create(itemType, materialType, quantity);
			m_items.push_back(&item);
		}
		//Invalidate move cache when impassably full.
		if(!impassible && block->impassible())
			block->m_hasActors.invalidateCache();
	}
	void remove(const ItemType& itemType, const MaterialType& materialType uint32_t quantity)
	{
		assert(itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		auto found = std::find(m_items, [&](Item* item) { return item.itemType == itemType && item.materialType == materialType; });
		assert(found != m_items.end());
		assert(found->quantity >= quantity);
		bool impassible = impassible();
		m_volume -= itemType.volume * quantity;
		// Remove all.
		if(found->quantity == quantity)
			m_items.erase(found);
		else
		{
			// Remove some.
			found->quantity -= quantity;
			found->reservable.setMaxReservations(found->quantity);
		}
		// Invalidate move cache when no-longer impassably full.
		if(impassible && !impassible())
			m_block.m_hasActors.invalidateCache();
	}
	void tranferTo(HasItems& other, Item& item)
	{
		other.add(item);
		remove(item);
	}
	void transferTo(HasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
	{
		other.add(itemType, materialType, quantity);
		remove(itemType, materialType, quantity
	}
	bool impassible() const { return m_volume >= Config::ImpassibleItemVolume; }
	Item* get(ItemType& itemType) const
	{
		auto found = std::ranges::find(m_items, itemType, &Item::ItemType);
		if(found == m_items.end())
			return nullptr;
		return &*found;
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
	uint32_t canPickupQuantityOf(const ItemType& itemType) const
	{
		return (m_actor.maxCarryMass - m_actor.m_totalCarryMass) / itemType.mass;
	}
};
