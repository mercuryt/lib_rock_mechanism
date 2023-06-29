#pragma once

#include "body.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <unordered_multimap>


struct AttackType
{
	std::string m_name;
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
	const std::string m_name;
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
		assert(m_volume <= m_item.m_itemType.internalVolume);
		bool combined = false;
		if(item.m_itemType.generic)
			for(auto& pair : m_itemsAndVolumes)
				if(pair.first()->genericsCanCombine(item))
				{
					pair.first.m_quantity += item.m_quantity;
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
	void remove(Item& item, uint32_t m_quantity)
	{
		assert(item.m_itemType.generic);
		assert(item.m_quantity >= m_quantity);
		item.m_quantity -= m_quantity;
		if(item.m_quantity == 0)
			m_itemsAndVolumes.remove(&item);
	}
	bool canAdd(Item& item) const
	{
		assert(&item != m_item);
		return m_fluidType == nullptr && m_volume + m_item.getVolume() <= m_item.m_itemType.internalVolume;
	}
	bool canAdd(FluidType& fluidType) const
	{
		return (m_fluidType == nullptr || m_fluidType == fluidType) && m_item.m_itemType.canHoldFluids && m_itemsAndVolumes.empty() && m_volume <= m_item.m_itemType.internalVolume;
	}
};
class Item : public HasShape
{
public:
	const uint32_t m_id;
	const ItemType& m_itemType;
	const MaterialType& m_materialType;
	uint32_t m_quantity; // Always set to 1 for nongeneric types.
	Reservable m_reservable;
	std::string m_name;
	uint32_t m_quality;
	uint32_t m_percentWear;
	bool m_installed;

	void setVolume() const { m_mass = m_quantity * m_itemType.volume; }
	void setMass() const 
	{ 
		m_mass = m_quantity * m_itemType.mass;
		if(m_itemType.internalVolume)
			for(Item* item : m_containsItemsOrFluids.getItems())
				m_mass += item.getMass();
       	}
	// Generic.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q):
		m_id(i), m_itemType(it), m_materialType(mt), m_quantity(q), m_reservable(q), m_installed(false)
	{
		assert(m_itemType.generic);
		mass = m_itemType.volume * m_materialType.mass * m_quantity;
		volume = m_itemType.volume * m_quantity;
	}
	// NonGeneric.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t qual, uint32_t pw):
		m_id(i), m_itemType(it), m_materialType(mt), m_quantity(1), m_quality(qual), m_reservable(1), m_percentWear(pw), m_installed(false)
	{
		assert(!m_itemType.generic);
		mass = m_itemType.volume * m_materialType.mass;
		volume = m_itemType.volume;
	// Named.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, uint32_t pw):
		m_id(i), m_itemType(it), m_materialType(mt), m_quantity(1), m_name(n), m_quality(qual), m_reservable(1), m_percentWear(pw), m_installed(false)
	{
		assert(!m_itemType.generic);
		mass = m_itemType.volume * m_materialType.mass;
		volume = m_itemType.volume;
	}
	static uint32_t s_nextId;
	struct Hash{ std::size_t operator()(const Item& item) const { return item.m_id; }};
	static std::unordered_set<Item, Hash> s_globalItems;
	// Generic items, created in local item set. 
	static Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity)
	{
		assert(m_itemType.generic);
		area.m_items.emplace(++s_nextId, m_itemType, m_materialType, m_quantity);
	}
	static Item& create(Area& area, const uint32_t m_id,  const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity)
	{
		assert(m_itemType.generic);
		if(!s_nextId > m_id) s_nextId = m_id + 1;
		area.m_items.emplace(m_id, m_itemType, m_materialType, m_quantity);
	}
	// Unnamed items, created in local item set.
	static Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear)
	{
		assert(!m_itemType.generic);
		area.m_items.emplace(++s_nextId, m_itemType, m_materialType, 1, "", m_quality, m_percentWear);
	}
	static Item& create(Area& area, const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear)
	{
		assert(!m_itemType.generic);
		if(!s_nextId > m_id) s_nextId = m_id + 1;
		area.m_items.emplace(m_id, m_itemType, m_materialType, 1, "", m_quality, m_percentWear);
	}
	// Named items, created in global item set.
	static Item& create(const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear)
	{
		assert(!m_itemType.generic);
		assert(!m_name.empty());
		s_globalItems.emplace(++s_nextId, m_itemType, m_materialType, 1, m_name, m_quality, m_percentWear);
	}
	static Item& create(const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear)
	{
		assert(!m_itemType.generic);
		assert(!m_name.empty());
		if(!s_nextId > m_id) s_nextId = m_id + 1;
		s_globalItems.emplace(m_id, m_itemType, m_materialType, 1, m_name, m_quality, m_percentWear);
	}
	//TODO: Items leave area.
};
Item::s_nextId = 0;
class ItemQuery
{
	Item* m_item;
	const ItemType* m_itemType;
	const MaterialTypeCategory* m_materialTypeCategory;
	const MaterialType* m_materialType;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(Item& item) : m_item(&item), m_itemType(nullptr), m_materialTypeCategory(nullptr), m_materialType(nullptr) { }
	ItemQuery(const ItemType& m_itemType) : m_item(nullptr), m_itemType(&m_itemType), MaterialTypeCategory(nullptr), m_materialType(nullptr) { }
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(mtc), m_materialType(nullptr) { }
	ItemQuery(const ItemType& m_itemType, const MaterialType& mt) : m_item(nullptr), m_itemType(&m_itemType), m_materialTypeCategory(nullptr), m_materialType(mt) { }
	bool operator()(Item& item) const
	{
		if(const m_item != nullptr)
			return item == m_item;
		if(m_itemType != item.m_itemType)
			return false;
		if(m_materialTypeCategory != nullptr)
			return m_materialTypeCategory == item.m_materialType.materialTypeCategory;
		if(m_materialType != nullptr)
			return m_materialType == item.m_materialType;
		return true;
	}
	void specalize(Item& item)
	{
		assert(m_itemType != nullptr && m_item == nullptr && item.m_itemType == m_itemType);
		m_item = item;
		m_itemType = nullptr;
	}
	void specalize(MaterialType& m_materialType)
	{
		assert(m_materialTypeCategory == nullptr || m_materialType.materialTypeCategory == m_materialTypeCategory);
		assert(m_materialType == nullptr);
		m_materialType = m_materialType;
		m_materialTypeCategory = nullptr;
	}
};
// To be used by actor and vehicle.
class HasItemsAndActors
{
	std::vector<Item*> m_items;
	std::vector<Actor*> m_actors;
	uint32_t m_mass;
	HasItems(): m_mass(0) { }
	// Non generic types have Shape.
	void add(Item& item)
	{
		assert(!item.m_itemType.generic);
		assert(std::ranges::find(m_items, &item) == m_items.end());
		m_mass += item.m_itemType.volume * item.m_materialType.mass;
		m_items.push_back(&item);
	}
	void remove(Item& item)
	{
		assert(!item.m_itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		m_mass -= item.m_itemType.volume * item.m_materialType.mass;
		std::erase(m_items, &item);
	}
	void add(const ItemType& m_itemType, const MaterialType& m_materialType uint32_t m_quantity)
	{
		assert(m_itemType.generic);
		m_mass += item.m_itemType.volume * item.m_materialType.mass * m_quantity;
		auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == m_itemType && item.m_materialType == m_materialType; });
		// Add to stack.
		if(found != m_items.end())
		{
			found->m_quantity += m_quantity;
			found->m_reservable.setMaxReservations(found->m_quantity);
		}
		// Create.
		else
		{
			Item& item = Item::create(m_itemType, m_materialType, m_quantity);
			m_items.push_back(&item);
		}
	}
	void remove(const ItemType& m_itemType, const MaterialType& m_materialType uint32_t m_quantity)
	{
		assert(m_itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		m_mass -= item.m_itemType.volume * item.m_materialType.mass * m_quantity;
		auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == m_itemType && item.m_materialType == m_materialType; });
		assert(found != m_items.end());
		assert(found->m_quantity >= m_quantity);
		// Remove all.
		if(found->m_quantity == m_quantity)
			m_items.erase(found);
		// Remove some.
		else
		{
			found->m_quantity -= m_quantity;
			found->m_reservable.setMaxReservations(found->m_quantity);
		}
	}
	template<class Other>
	void tranferTo(Other& other, Item& item)
	{
		other.add(item);
		remove(item);
	}
	template<class Other>
	void transferTo(Other& other, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity)
	{
		other.add(m_itemType, m_materialType, m_quantity);
		remove(m_itemType, m_materialType, m_quantity
	}
	Item* get(ItemType& itemType) const
	{
		auto found = std::ranges::find(m_items, itemType, &Item::ItemType);
		if(found == m_items.end())
			return nullptr;
		return &*found;
	}
	void add(Actor& actor)
	{
		assert(std::ranges::find(m_actors, &actor) == m_actors.end());
		m_mass += actor.getMass();
		m_actors.push_back(&actor);
	}
	void remove(Actor& actor)
	{
		assert(std::ranges::find(m_actors, &actor) != m_actors.end());
		m_mass -= actor.getMass();
		m_actors.push_back(&actor);
	}
	template<class Other>
	void transferTo(Other& other, Actor& actor)
	{
		assert(std::ranges::find(m_actors, &actor) != m_actors.end());
		remove(&actor);
		other.add(actor);
	}
};
class BlockHasItems
{
	Block& m_block;
	std::vector<Item*> m_items;
	uint32_t m_volume;
	BlockHasItems(Block& b): m_volume(0) , m_block(b) { }
	// Non generic types have Shape.
	void add(Item& item)
	{
		assert(!item.m_itemType.generic);
		assert(std::ranges::find(m_items, &item) == m_items.end());
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : item.m_itemType.shape.positions)
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
		assert(!item.m_itemType.generic);
		assert(std::ranges::find(m_items, &item) != m_items.end());
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : item.m_itemType.shape.positions)
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
		auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == itemType && item.m_materialType == materialType; });
		bool impassible = block->impassible();
		m_volume += itemType.volume * quantity;
		// Add to.
		if(found != m_items.end())
		{
			found->m_quantity += quantity;
			found->m_reservable.setMaxReservations(found->m_quantity);
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
		auto found = std::find(m_items, [&](Item* item) { return item.m_itemType == itemType && item.m_materialType == materialType; });
		assert(found != m_items.end());
		assert(found->m_quantity >= quantity);
		bool impassible = impassible();
		m_volume -= itemType.volume * quantity;
		// Remove all.
		if(found->m_quantity == quantity)
			m_items.erase(found);
		else
		{
			// Remove some.
			found->m_quantity -= quantity;
			found->m_reservable.setMaxReservations(found->m_quantity);
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
class BlockHasItemsAndActors
{
	Block& m_block;
public:
	void add(Item& item) { m_block.m_hasItems.add(item); }
	void remove(Item& item) { m_block.m_hasItems.remove(item); }
	void add(Actor& actor) { m_block.m_hasActors.add(actor); }
	void remove(Actor& actor) { m_block.m_hasActors.remove(actor); }
	template<class Other, class ToTransfer>
	void transferTo(Other& other, ToTransfer& toTransfer)
	{
		other.add(toTransfer);
		remove(toTransfer);
	}
};
class CanPickup
{
	Actor& m_actor;
	HasItemsAndActors m_hasItemsAndActors;
	HasShape* m_carrying;
	void pickUp(Item& item, uint8_t quantity)
	{
		assert(quantity <= item.m_quantity);
		assert(m_carrying = nullptr);
		m_actor.m_location->m_hasItemsAndActors.tranferTo(m_hasItemsAndActors, item, quantity)
		item.m_reservable.unreserve(m_actor.canReserve);
		m_actor.setCarryMass();
		m_carrying = item;
	}
	void pickUp(Actor& actor)
	{
		assert(!actor.isConcious() || actor.isInjured());
		assert(m_carrying = nullptr);
		m_hasItemsAndActors.add(actor);
		actor.m_reservable.unreserve(m_actor.canReserve);
		m_carrying = actor;
		m_actor.setCarryMass();
	}
	void putDown()
	{
		assert(m_carrying != nullptr);
		if(m_carrying.isItem())
			m_hasItemsAndActors.transferTo(m_actor.m_location->m_hasItems, *m_carrying);
		else
			m_hasItemsAndActors.transferTo(m_actor.m_location->m_hasActors, *m_carrying);
		m_carrying = nullptr;
		m_actor.setCarryMass();
	}
	uint32_t canPickupQuantityOf(const ItemType& itemType) const
	{
		return (m_actor.maxCarryMass - m_actor.m_totalCarryMass) / itemType.mass;
	}
};
