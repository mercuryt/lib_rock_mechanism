#pragma once

#include "materialType.h"
#include "hasShape.h"
#include "reservable.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct BodyPartType;
struct WeaponType;
struct Item;
class Area;
class Actor;

struct WearableType
{
	const std::string name;
	const uint32_t percentCoverage;
	const uint32_t defenseScore;
	const bool rigid;
	const uint32_t impactSpreadArea;
	const uint32_t forceAbsorbedPiercedModifier;
	const uint32_t forceAbsorbedUnpiercedModifier;
	const std::unordered_set<BodyPartType*> bodyPartsCovered;
	// Infastructure.
	bool operator==(const WearableType& wearableType){ return this == &wearableType; }
	static std::vector<WearableType> data;
	static const WearableType& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &WearableType::name);
		assert(found != data.end());
		return *found;
	}
};
struct ItemType
{
	const std::string name;
	const bool installable;
	const Shape& shape;
	const bool generic;
	const uint32_t internalVolume;
	const bool canHoldFluids;
	const WeaponType* weaponType;
       	const WearableType* wearableType;
	// Infastructure.
	bool operator==(const ItemType& itemType){ return this == &itemType; }
	static std::vector<ItemType> data;
	static const ItemType& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &ItemType::name);
		assert(found != data.end());
		return *found;
	}
};
class ItemHasCargo
{
	Item& m_item;
	uint32_t m_volume;
	uint32_t m_mass;
	std::unordered_map<HasShape*, uint32_t> m_volumes;
	const FluidType* m_fluidType;
public:
	ItemHasCargo(Item* i);
	void add(HasShape& hasShape);
	void add(const FluidType& fluidType, uint32_t volume);
	void remove(const FluidType& fluidType, uint32_t volume);
	void remove(HasShape& hasShape);
	void remove(Item& item, uint32_t m_quantity);
	bool canAdd(HasShape& hasShape) const;
	bool canAdd(FluidType& fluidType) const;
};
class Item : public HasShape
{
	inline static uint32_t s_nextId = 1;
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
	ItemHasCargo m_hasCargo; //TODO: Change to unique_ptr to save some RAM?

	void setVolume() const;
	void setMass() const;
	// Generic.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q);
	// NonGeneric.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t qual, uint32_t pw);
	// Named.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, uint32_t pw);
	struct Hash { std::size_t operator()(Item& item); };
	static std::unordered_set<Item, Hash> s_globalItems;
	// Generic items, created in local item set. 
	static Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity);
	static Item& create(Area& area, const uint32_t m_id,  const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity);
	// Unnamed items, created in local item set.
	static Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear);
	static Item& create(Area& area, const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear);
	// Named items, created in global item set.
	static Item& create(const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear);
	static Item& create(const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear);
	//TODO: Items leave area.
};
class ItemQuery
{
	Item* m_item;
	const ItemType* m_itemType;
	const MaterialTypeCategory* m_materialTypeCategory;
	const MaterialType* m_materialType;
public:
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(Item& item);
	ItemQuery(const ItemType& m_itemType);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc);
	ItemQuery(const ItemType& m_itemType, const MaterialType& mt);
	bool operator()(Item& item) const;
	void specalize(Item& item);
	void specalize(MaterialType& m_materialType);
};
// To be used by actor and vehicle.
class HasItemsAndActors
{
	std::vector<Item*> m_items;
	std::vector<Actor*> m_actors;
	uint32_t m_mass;
public:
	// Non generic types have Shape.
	void add(Item& item);
	void remove(Item& item);
	void add(const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity);
	void remove(const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity);
	template<class Other>
	void tranferTo(Other& other, Item& item);
	template<class Other>
	void transferTo(Other& other, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity);
	Item* get(ItemType& itemType) const;
	void add(Actor& actor);
	void remove(Actor& actor);
	template<class Other>
	void transferTo(Other& other, Actor& actor);
};
class BlockHasItems
{
	Block& m_block;
	std::vector<Item*> m_items;
	uint32_t m_volume;
	BlockHasItems(Block& b);
	// Non generic types have Shape.
	void add(Item& item);
	void remove(Item& item);
	void add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void remove(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	template<class HasItems>
	void tranferTo(HasItems& other, Item& item);
	template<class HasItems>
	void transferTo(HasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	bool impassible() const;
	Item* get(ItemType& itemType) const;
};
class BlockHasItemsAndActors
{
	Block& m_block;
public:
	BlockHasItemsAndActors(Block& b) : m_block(b) { }
	void add(Item& item);
	void remove(Item& item);
	void add(Actor& actor);
	void remove(Actor& actor);
	template<class Other, class ToTransfer>
	void transferTo(Other& other, ToTransfer& toTransfer);
};
