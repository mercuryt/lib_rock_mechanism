#pragma once

#include "materialType.h"
#include "hasShape.h"
#include "reservable.h"
#include "attackType.h"
#include "fluidType.h"
#include "move.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct BodyPartType;
struct WeaponType;
class Item;
class Area;
class Actor;
struct CraftJob;

struct WearableType
{
	const std::string name;
	const uint32_t percentCoverage;
	const uint32_t defenseScore;
	const bool rigid;
	const uint32_t impactSpreadArea;
	const uint32_t forceAbsorbedPiercedModifier;
	const uint32_t forceAbsorbedUnpiercedModifier;
	std::vector<const BodyPartType*> bodyPartsCovered;
	uint32_t layer;
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
	const uint32_t volume;
	const bool generic;
	const uint32_t internalVolume;
	const bool canHoldFluids;
       	const WearableType* wearableType;
	const WeaponType* weaponType;
	const uint32_t combatScore;
	const MoveType& moveType;
	// Infastructure.
	bool operator==(const ItemType& itemType) const { return this == &itemType; }
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
	std::vector<HasShape*> m_shapes;
	std::vector<Item*> m_items;
	const FluidType* m_fluidType;
	uint32_t m_fluidVolume;
public:
	ItemHasCargo(Item& i) : m_item(i) { }
	void add(HasShape& hasShape);
	void add(const FluidType& fluidType, uint32_t volume);
	void remove(const FluidType& fluidType, uint32_t volume);
	void remove(HasShape& hasShape);
	void remove(Item& item, uint32_t quantity);
	std::vector<HasShape*>& getContents() { return m_shapes; }
	std::vector<Item*>& getItems() { return m_items; }
	bool canAdd(HasShape& hasShape) const;
	bool canAdd(FluidType& fluidType) const;
	const uint32_t& getFluidVolume() const { return m_fluidVolume; }
	const FluidType& getFluidType() const { return *m_fluidType; }
	bool containsAnyFluid() const { return m_fluidType != nullptr; }
	bool contains(HasShape& hasShape) const { return std::ranges::find(m_shapes, &hasShape) != m_shapes.end(); }
};
class Item : public HasShape
{
	inline static uint32_t s_nextId = 1;
public:
	const uint32_t m_id;
	const ItemType& m_itemType;
	const MaterialType& m_materialType;
	uint32_t m_mass;
	uint32_t m_volume;
	uint32_t m_quantity; // Always set to 1 for nongeneric types.
	Reservable m_reservable;
	std::string m_name;
	uint32_t m_quality;
	uint32_t m_percentWear;
	bool m_installed;
	CraftJob* m_craftJobForWorkPiece; // Used only for work in progress items.
	ItemHasCargo m_hasCargo; //TODO: Change to unique_ptr to save some RAM?
	//TODO: ItemHasOwners

	// Generic.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t q, CraftJob* cj);
	// NonGeneric.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, uint32_t qual, uint32_t pw, CraftJob* cj);
	// Named.
	Item(uint32_t i, const ItemType& it, const MaterialType& mt, std::string n, uint32_t qual, uint32_t pw, CraftJob* cj);
	void setVolume();
	void setMass();
	void destroy();
	void setLocation(Block& block);
	void exit();
	void pierced(uint32_t area);
	bool isItem() const { return true; }
	bool isActor() const { return false; }
	bool edible() const;
	bool isPreparedMeal() const;
	uint32_t singleUnitMass() const { return m_itemType.volume * m_materialType.density; }
	uint32_t getMass() const { return m_quantity * singleUnitMass(); }
	uint32_t getVolume() const { return m_quantity * m_itemType.volume; }
	const MoveType& getMoveType() const { return m_itemType.moveType; }
	static std::list<Item> s_globalItems;
	// Generic items, created in local item set. 
	static Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity, CraftJob* cj = nullptr);
	static Item& create(Area& area, const uint32_t m_id,  const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quantity, CraftJob* cj = nullptr);
	// Unnamed items, created in local item set.
	static Item& create(Area& area, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear, CraftJob* cj = nullptr);
	static Item& create(Area& area, const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, uint32_t m_quality, uint32_t m_percentWear, CraftJob* cj = nullptr);
	// Named items, created in global item set.
	static Item& create(const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear, CraftJob* cj = nullptr);
	static Item& create(const uint32_t m_id, const ItemType& m_itemType, const MaterialType& m_materialType, std::string m_name, uint32_t m_quality, uint32_t m_percentWear, CraftJob* cj = nullptr);
	//TODO: Items leave area.
};
class ItemQuery
{
public:
	Item* m_item;
	const ItemType* m_itemType;
	const MaterialTypeCategory* m_materialTypeCategory;
	const MaterialType* m_materialType;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(Item& item);
	ItemQuery(const ItemType& m_itemType);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc);
	ItemQuery(const ItemType& m_itemType, const MaterialType& mt);
	bool operator()(const Item& item) const;
	void specalize(Item& item);
	void specalize(const MaterialType& materialType);
};
class BlockHasItems
{
	Block& m_block;
	std::vector<Item*> m_items;
public:
	BlockHasItems(Block& b);
	// Non generic types have Shape.
	void add(Item& item);
	void remove(Item& item);
	void add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void remove(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	//Item* get(ItemType& itemType) const;
	std::vector<Item*>& getAll() const;
	bool hasInstalledItemType(const ItemType& itemType) const;
	bool hasEmptyContainerWhichCanHoldFluidsCarryableBy(Actor& actor) const;
	bool hasContainerContainingFluidTypeCarryableBy(Actor& actor, const FluidType& fluidType) const;
	bool empty() const { return m_items.empty(); }
};
