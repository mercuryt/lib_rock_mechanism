#pragma once

#include "attackType.h"
#include "blocks/blocks.h"
#include "eventSchedule.hpp"
#include "hasShape.h"
#include "types.h"

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
	std::vector<const BodyPartType*> bodyPartsCovered;
	const uint32_t defenseScore = 0;
	const uint32_t layer = 0;
	const uint32_t bodyTypeScale = 0;
	const uint32_t forceAbsorbedUnpiercedModifier = 0;
	const uint32_t forceAbsorbedPiercedModifier = 0;
	const Percent percentCoverage = 0;
	const bool rigid = false;
	WearableData(uint32_t ds, uint32_t l, uint32_t bts, uint32_t faum, uint32_t fapm, Percent pc, bool r) : 
		defenseScore(ds), layer(l), bodyTypeScale(bts), forceAbsorbedUnpiercedModifier(faum), forceAbsorbedPiercedModifier(fapm), 
		percentCoverage(pc), rigid(r) { }
};
inline std::deque<WearableData> wearableDataStore;
struct WeaponData final
{
	std::vector<AttackType> attackTypes;
	const SkillType* combatSkill;
	Step coolDown;
	WeaponData(const SkillType* cs, Step cd) : combatSkill(cs), coolDown(cd)  { }
	// Infastructure.
};
inline std::deque<WeaponData> weaponDataStore;
struct ItemType final
{
	std::vector<const MaterialTypeCategory*> materialTypeCategories;
	const std::string name;
	//TODO: remove craftLocationOffset?
	const std::array<int32_t, 3> craftLocationOffset = {0,0,0};
	const Shape& shape;
	const MoveType& moveType;
	const FluidType* edibleForDrinkersOf = nullptr;
       	const WearableData* wearableData = nullptr;
       	const WeaponData* weaponData = nullptr;
	const CraftStepTypeCategory* craftLocationStepTypeCategory = nullptr;
	const Volume volume = 0;
	const Volume internalVolume = 0;
	const uint32_t value = 0;
	const bool installable = false;
	const bool generic = false;
	const bool canHoldFluids = false;
	ItemType(std::string name, const Shape& shape, const MoveType& moveType, Volume volume, Volume internalVolume, uint32_t value, bool installable, bool generic, bool canHoldFluids);
	AttackType* getRangedAttackType() const;
	BlockIndex getCraftLocation(Blocks& blocks, BlockIndex location, Facing facing) const;
	bool hasRangedAttack() const;
	bool hasMeleeAttack() const;
	// Infastructure.
	bool operator==(const ItemType& itemType) const { return this == &itemType; }
	static const ItemType& byName(std::string name);
	static ItemType& byNameNonConst(std::string name);
};
inline std::vector<ItemType> itemTypeDataStore;
inline void to_json(Json& data, const ItemType* const& itemType){ data = itemType->name; }
inline void to_json(Json& data, const ItemType& itemType){ data = itemType.name; }
inline void from_json(const Json& data, const ItemType*& itemType){ itemType = &ItemType::byName(data.get<std::string>()); }
struct ItemParamaters final
{
	Simulation* simulation = nullptr;
	ItemId id = 0;
	const ItemType& itemType;
	const MaterialType& materialType;
	Quantity quantity = 1;
	uint32_t quality = 0;
	Percent percentWear = 0;
	CraftJob* craftJob = nullptr;
	BlockIndex location = BLOCK_INDEX_MAX;
	Area* area = nullptr;
	Faction* faction = nullptr;
	bool installed = false;
	ItemId getId();
};
class ItemHasCargo final
{
	std::vector<HasShape*> m_shapes;
	std::vector<Item*> m_items;
	Item& m_item;
	const FluidType* m_fluidType = nullptr;
	Volume m_volume = 0;
	Mass m_mass = 0;
	Volume m_fluidVolume = 0;
public:
	ItemHasCargo(Item& i) : m_item(i) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void add(HasShape& hasShape);
	void add(const FluidType& fluidType, Volume volume);
	Item& add(const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void remove(const FluidType& fluidType, Volume volume);
	void removeFluidVolume(Volume volume);
	void remove(HasShape& hasShape);
	void remove(Item& item, Quantity quantity);
	void remove(const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void load(HasShape& hasShape, Quantity quantity = 0);
	void unloadTo(HasShape& hasShape, BlockIndex location);
	Item& unloadGenericTo(const ItemType& itemType, const MaterialType& materialType, Quantity quantity, BlockIndex location);
	void destroyCargo(Item& item);
	[[nodiscard]] std::vector<HasShape*>& getContents() { return m_shapes; }
	[[nodiscard]] std::vector<Item*>& getItems() { return m_items; }
	[[nodiscard]] std::vector<Actor*> getActors();
	[[nodiscard]] const std::vector<Item*>& getItems() const { return m_items; }
	[[nodiscard]] bool canAdd(HasShape& hasShape) const;
	[[nodiscard]] bool canAdd(FluidType& fluidType) const;
	[[nodiscard]] const Volume& getFluidVolume() const { return m_fluidVolume; }
	[[nodiscard]] const FluidType& getFluidType() const { assert(m_fluidType); return *m_fluidType; }
	[[nodiscard]] bool containsAnyFluid() const { return m_fluidType != nullptr; }
	[[nodiscard]] bool contains(HasShape& hasShape) const { return std::ranges::find(m_shapes, &hasShape) != m_shapes.end(); }
	[[nodiscard]] bool containsGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const;
	[[nodiscard]] bool empty() const { return m_fluidType == nullptr && m_shapes.empty(); }
};
class ReMarkItemForStockPilingEvent final : public ScheduledEvent
{
	Item& m_item;
	Faction& m_faction;
public:
	ReMarkItemForStockPilingEvent(Item& i, Faction& f, Step duration, const Step start = 0);
	void execute();
	void clearReferences();
};
//TODO: Flat set.
//TODO: Change from linear to geometric delay duration.
class ItemCanBeStockPiled
{
	std::unordered_set<Faction*> m_data;
	std::unordered_map<Faction*, HasScheduledEvent<ReMarkItemForStockPilingEvent>> m_scheduledEvents;
	Item& m_item;
	void scheduleReset(Faction& faction, Step duration, Step start = 0);
public:
	ItemCanBeStockPiled(Item& i) : m_item(i) { }
	ItemCanBeStockPiled(const Json& data, DeserializationMemo& deserializationMemo, Item& item);
	Json toJson() const;
	void set(Faction& faction) { assert(!m_data.contains(&faction)); m_data.insert(&faction); }
	void maybeSet(Faction& faction) { m_data.insert(&faction); }
	void unset(Faction& faction) { assert(m_data.contains(&faction)); m_data.erase(&faction); }
	void maybeUnset(Faction& faction) { m_data.erase(&faction); }
	void unsetAndScheduleReset(Faction& faction, Step duration) { unset(faction); scheduleReset(faction, duration); }
	void maybeUnsetAndScheduleReset(Faction& faction, Step duration)  { maybeUnset(faction); scheduleReset(faction, duration); }
	[[nodiscard]] bool contains(Faction& faction) { return m_data.contains(&faction); }
	friend class ReMarkItemForStockPilingEvent;
};
class Item final : public HasShape
{
public:
	ItemHasCargo m_hasCargo; // 7.5
	ItemCanBeStockPiled m_canBeStockPiled; // 5
	std::wstring m_name;
	CraftJob* m_craftJobForWorkPiece; // Used only for work in progress items.
	const ItemType& m_itemType;
	const MaterialType& m_materialType;
	const ItemId m_id;
	uint32_t m_quality;
	//TODO: Percent doesn't allow fine enough detail for tools wearing out over time?
	Percent m_percentWear;
private:
	Quantity m_quantity; // Always set to 1 for nongeneric types.
public:
	bool m_installed;
	//TODO: ItemHasOwners
	Item(ItemParamaters itemParamaters);
	// Generic.
	Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, Quantity q, CraftJob* cj);
	// NonGeneric.
	Item(Simulation& s, ItemId i, const ItemType& it, const MaterialType& mt, uint32_t qual, Percent pw, CraftJob* cj);
	Item(const Json& data, DeserializationMemo& deserializationMemo, ItemId i);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void setName(std::wstring name) { m_name = name; }
	void destroy();
	void setLocation(BlockIndex block, Area* area = nullptr);
	void exit();
	void pierced(Volume volume);
	void setTemperature(Temperature temperature);
	void addQuantity(Quantity delta);
	void removeQuantity(Quantity delta);
	void install(BlockIndex block, Facing facing, Faction& faction);
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
