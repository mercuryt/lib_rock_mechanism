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
	ItemContainsItemsOrFluids(Item* i);
	void add(Item& item);
	void add(const FluidType& fluidType, uint32_t volume);
	void remove(const FluidType& fluidType, uint32_t volume);
	void remove(Item& item);
	void remove(Item& item, uint32_t m_quantity);
	bool canAdd(Item& item) const;
	bool canAdd(FluidType& fluidType) const;
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
	ItemHasCargo m_hasCargo; //TODO: Change to unique_ptr to save some RAM?

	void setVolume() const;
	void setMass() const;
	// Generic.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q);
	// NonGeneric.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t qual, uint32_t pw);
	// Named.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, uint32_t pw);
	static uint32_t s_nextId;
	struct Hash{ std::size_t operator();};
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
Item::s_nextId = 0;
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
	void add(const ItemType& m_itemType, const MaterialType& m_materialType uint32_t m_quantity);
	void remove(const ItemType& m_itemType, const MaterialType& m_materialType uint32_t m_quantity);
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
	BlockHasItems(Block& b;
	// Non generic types have Shape.
	void add(Item& item);
	void remove(Item& item);
	void add(const ItemType& itemType, const MaterialType& materialType uint32_t quantity);
	void remove(const ItemType& itemType, const MaterialType& materialType uint32_t quantity);
	void tranferTo(HasItems& other, Item& item);
	void transferTo(HasItems& other, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	bool impassible() const;
	Item* get(ItemType& itemType) const;
};
class BlockHasItemsAndActors
{
	Block& m_block;
public:
	void add(Item& item);
	void remove(Item& item);
	void add(Actor& actor);
	void remove(Actor& actor);
	template<class Other, class ToTransfer>
	void transferTo(Other& other, ToTransfer& toTransfer);
};
