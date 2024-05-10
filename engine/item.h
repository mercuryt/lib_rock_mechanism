#pragma once

#include "eventSchedule.h"
#include "eventSchedule.hpp"
#include "hasShape.h"
#include "attackType.h"

#include <ratio>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct BodyPartType;
struct BodyTypeCategory;
class Item;
class Area;
class Actor;
struct CraftJob;
struct CraftStepTypeCategory;
struct FluidType;
struct MaterialType;
struct DeserializationMemo;
struct MaterialTypeCategory;

struct WearableData final
{
	const Percent percentCoverage;
	const uint32_t defenseScore;
	const bool rigid;
	const uint32_t layer;
	const uint32_t bodyTypeScale;
	const uint32_t forceAbsorbedUnpiercedModifier;
	const uint32_t forceAbsorbedPiercedModifier;
	std::vector<const BodyPartType*> bodyPartsCovered;
	// Infastructure.
};
inline std::deque<WearableData> wearableDataStore;
struct WeaponData final
{
	const SkillType* combatSkill;
	Step coolDown;
	std::vector<AttackType> attackTypes;
	// Infastructure.
};
inline std::deque<WeaponData> weaponDataStore;
struct ItemType final
{
	const std::string name;
	const bool installable;
	const Shape& shape;
	const Volume volume;
	const bool generic;
	const Volume internalVolume;
	const bool canHoldFluids;
	const uint32_t value;
	const FluidType* edibleForDrinkersOf;
	const MoveType& moveType;
       	const WearableData* wearableData;
       	const WeaponData* weaponData;
	const std::array<int32_t, 3> craftLocationOffset;
	const CraftStepTypeCategory* craftLocationStepTypeCategory;
	std::vector<const MaterialTypeCategory*> materialTypeCategories;
	AttackType* getRangedAttackType() const;
	bool hasRangedAttack() const;
	bool hasMeleeAttack() const;
	Block* getCraftLocation(const Block& location, Facing facing) const;
	// Infastructure.
	bool operator==(const ItemType& itemType) const { return this == &itemType; }
	static const ItemType& byName(std::string name);
	static ItemType& byNameNonConst(std::string name);
};
inline std::vector<ItemType> itemTypeDataStore;
inline void to_json(Json& data, const ItemType* const& itemType){ data = itemType->name; }
inline void to_json(Json& data, const ItemType& itemType){ data = itemType.name; }
inline void from_json(const Json& data, const ItemType*& itemType){ itemType = &ItemType::byName(data.get<std::string>()); }
class ItemHasCargo final
{
	Item& m_item;
	Volume m_volume = 0;
	Mass m_mass = 0;
	std::vector<HasShape*> m_shapes;
	std::vector<Item*> m_items;
	const FluidType* m_fluidType = nullptr;
	Volume m_fluidVolume = 0;
public:
	ItemHasCargo(Item& i) : m_item(i) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void add(HasShape& hasShape);
	void add(const FluidType& fluidType, Volume volume);
	Item& add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void remove(const FluidType& fluidType, Volume volume);
	void removeFluidVolume(Volume volume);
	void remove(HasShape& hasShape);
	void remove(Item& item, uint32_t quantity);
	void remove(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity);
	void load(HasShape& hasShape, uint32_t quantity = 0);
	void unloadTo(HasShape& hasShape, Block& location);
	Item& unloadGenericTo(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, Block& location);
	std::vector<HasShape*>& getContents() { return m_shapes; }
	std::vector<Item*>& getItems() { return m_items; }
	std::vector<Actor*> getActors();
	const std::vector<Item*>& getItems() const { return m_items; }
	bool canAdd(HasShape& hasShape) const;
	bool canAdd(FluidType& fluidType) const;
	const Volume& getFluidVolume() const { return m_fluidVolume; }
	const FluidType& getFluidType() const { assert(m_fluidType); return *m_fluidType; }
	bool containsAnyFluid() const { return m_fluidType != nullptr; }
	bool contains(HasShape& hasShape) const { return std::ranges::find(m_shapes, &hasShape) != m_shapes.end(); }
	bool containsGeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity) const;
	bool empty() const { return m_fluidType == nullptr && m_shapes.empty(); }
};
class ReMarkItemForStockPilingEvent final : public ScheduledEvent
{
	Item& m_item;
	const Faction& m_faction;
public:
	ReMarkItemForStockPilingEvent(Item& i, const Faction& f, Step duration, const Step start = 0);
	void execute();
	void clearReferences();
};
//TODO: Flat set.
//TODO: Change from linear to geometric delay duration.
class ItemCanBeStockPiled
{
	Item& m_item;
	std::unordered_set<const Faction*> m_data;
	std::unordered_map<const Faction*, HasScheduledEvent<ReMarkItemForStockPilingEvent>> m_scheduledEvents;
	void scheduleReset(const Faction& faction, Step duration, Step start = 0);
public:
	ItemCanBeStockPiled(Item& i) : m_item(i) { }
	ItemCanBeStockPiled(const Json& data, DeserializationMemo& deserializationMemo, Item& item);
	Json toJson() const;
	void set(const Faction& faction) { assert(!m_data.contains(&faction)); m_data.insert(&faction); }
	void maybeSet(const Faction& faction) { m_data.insert(&faction); }
	void unset(const Faction& faction) { assert(m_data.contains(&faction)); m_data.erase(&faction); }
	void maybeUnset(const Faction& faction) { m_data.erase(&faction); }
	void unsetAndScheduleReset(const Faction& faction, Step duration) { unset(faction); scheduleReset(faction, duration); }
	void maybeUnsetAndScheduleReset(const Faction& faction, Step duration)  { maybeUnset(faction); scheduleReset(faction, duration); }
	[[nodiscard]] bool contains(const Faction& faction) { return m_data.contains(&faction); }
	friend class ReMarkItemForStockPilingEvent;
};
class Item final : public HasShape
{
	uint32_t m_quantity; // Always set to 1 for nongeneric types.
public:
	const ItemId m_id;
	const ItemType& m_itemType;
	const MaterialType& m_materialType;
	Mass m_mass;
	Volume m_volume;
	std::wstring m_name;
	uint32_t m_quality;
	//TODO: Percent doesn't allow fine enough detail for tools wearing out over time?
	Percent m_percentWear;
	bool m_installed;
	CraftJob* m_craftJobForWorkPiece; // Used only for work in progress items.
	ItemHasCargo m_hasCargo; //TODO: Change to reference to save some RAM?
	ItemCanBeStockPiled m_canBeStockPiled;
	//TODO: ItemHasOwners
	// Generic.
	Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, uint32_t q, CraftJob* cj);
	// NonGeneric.
	Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, uint32_t qual, Percent pw, CraftJob* cj);
	Item(const Json& data, DeserializationMemo& deserializationMemo, ItemId i);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void setName(std::wstring name) { m_name = name; }
	void setVolume();
	void setMass();
	void destroy();
	void setLocation(Block& block);
	void exit();
	void pierced(Area area);
	void setTemperature(Temperature temperature);
	void addQuantity(uint32_t delta);
	void removeQuantity(uint32_t delta);
	void install(Block& block, Facing facing, const Faction& faction);
	void merge(Item& item);
	[[nodiscard]] Quantity getQuantity() const { return m_quantity; }
	[[nodiscard]] bool isItem() const { return true; }
	[[nodiscard]] bool isGeneric() const { return m_itemType.generic; }
	[[nodiscard]] bool isActor() const { return false; }
	[[nodiscard]] bool isPreparedMeal() const;
	[[nodiscard]] bool isWorkPiece() const { return m_craftJobForWorkPiece != nullptr; }
	Mass singleUnitMass() const;
	Mass getMass() const { return m_quantity * singleUnitMass(); }
	Volume getVolume() const { return m_quantity * m_itemType.volume; }
	const MoveType& getMoveType() const { return m_itemType.moveType; }
	bool operator==(const Item& item) const { return this == &item; }
	Item(const Item&) = delete;
	Item(const Item&&) = delete;
	//TODO: Items leave area.
	// For debugging.
	void log() const;
};
inline void to_json(Json& data, Item* const& item){ data = item->m_id; }
inline void to_json(Json& data, const Item& item){ data = item.m_id; }
