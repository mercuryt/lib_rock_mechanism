#pragma once

#include "bloomHashMap.h"
#include "portables.h"
#include "types.h"

struct ItemType;
struct MaterialType;
struct FluidType;
class ItemHasCargo;
class ItemCanBeStockPiled;
class CraftJob;
class Area;

struct ItemParamaters final
{
	const ItemType& itemType;
	const MaterialType& materialType;
	CraftJob* craftJob = nullptr;
	Faction* faction = nullptr;
	BlockIndex location = BLOCK_INDEX_MAX;
	ItemId id = 0;
	Quantity quantity = 1;
	uint32_t quality = 0;
	Percent percentWear = 0;
	Facing facing = 0;
	bool isStatic = true;
	bool installed = false;
};
class ReMarkItemForStockPilingEvent final : public ScheduledEvent
{
	Faction& m_faction;
	ItemCanBeStockPiled& m_canBeStockPiled;
public:
	ReMarkItemForStockPilingEvent(ItemCanBeStockPiled& i, Faction& f, Step duration, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
//TODO: Flat set.
//TODO: Change from linear to geometric delay duration.
class ItemCanBeStockPiled
{
	Area& m_area;
	std::unordered_map<Faction*, HasScheduledEvent<ReMarkItemForStockPilingEvent>> m_scheduledEvents;
	std::unordered_set<Faction*> m_data;
	void scheduleReset(Faction& faction, Step duration, Step start = 0);
public:
	ItemCanBeStockPiled(Area& area) : m_area(area) { }
	ItemCanBeStockPiled(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void set(Faction& faction) { assert(!m_data.contains(&faction)); m_data.insert(&faction); }
	void maybeSet(Faction& faction) { m_data.insert(&faction); }
	void unset(Faction& faction) { assert(m_data.contains(&faction)); m_data.erase(&faction); }
	void maybeUnset(Faction& faction) { m_data.erase(&faction); }
	void unsetAndScheduleReset(Faction& faction, Step duration) { unset(faction); scheduleReset(faction, duration); }
	void maybeUnsetAndScheduleReset(Faction& faction, Step duration)  { maybeUnset(faction); scheduleReset(faction, duration); }
	[[nodiscard]] bool contains(const Faction& faction) const { return m_data.contains(&const_cast<Faction&>(faction)); }
	friend class ReMarkItemForStockPilingEvent;
};
class ItemHasCargo final
{
	std::vector<ActorIndex> m_actors;
	std::vector<ItemIndex> m_items;
	const FluidType* m_fluidType = nullptr;
	const ItemType& m_itemType;
	Volume m_volume = 0;
	Mass m_mass = 0;
	Volume m_fluidVolume = 0;
public:
	ItemHasCargo(const ItemType& itemType) : m_itemType(itemType) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addItem(Area& area, ItemIndex itemIndex);
	void addActor(Area& area, ActorIndex actorIndex);
	void addFluid(const FluidType& fluidType, Volume volume);
	ItemIndex addItemGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void removeFluidVolume(const FluidType& fluidType, Volume volume);
	void removeActor(Area& area, ActorIndex actorIndex);
	void removeItem(Area& area, ItemIndex item);
	void removeItemGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	ItemIndex unloadGenericTo(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity, BlockIndex location);
	[[nodiscard]] std::vector<ActorIndex>& getActors() { return m_actors; }
	[[nodiscard]] std::vector<ItemIndex>& getItems() { return m_items; }
	[[nodiscard]] const std::vector<ItemIndex>& getItems() const { return m_items; }
	[[nodiscard]] bool canAddActor(Area& area, ActorIndex index) const;
	[[nodiscard]] bool canAddItem(Area& area, ItemIndex item) const;
	[[nodiscard]] bool canAddFluid(FluidType& fluidType) const;
	[[nodiscard]] const Volume& getFluidVolume() const { return m_fluidVolume; }
	[[nodiscard]] const FluidType& getFluidType() const { assert(m_fluidType); return *m_fluidType; }
	[[nodiscard]] bool containsAnyFluid() const { return m_fluidType != nullptr; }
	[[nodiscard]] bool containsFluidType(const FluidType& fluidType) const { return m_fluidType == &fluidType; }
	[[nodiscard]] bool containsActor(ActorIndex index) const { return std::ranges::find(m_actors, index) != m_actors.end(); }
	[[nodiscard]] bool containsItem(ItemIndex index) const { return std::ranges::find(m_items, index) != m_items.end(); }
	[[nodiscard]] bool containsGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const;
	[[nodiscard]] bool empty() const { return m_fluidType == nullptr && m_actors.empty() && m_items.empty(); }
	[[nodiscard]] Mass getMass() const { return m_mass; }
};
class Items final : public Portables
{
	//TODO: change to bitset or remove.
	std::unordered_set<ItemIndex> m_onSurface;
	BloomHashMap<ItemIndex, std::wstring> m_name;
	std::vector<std::unique_ptr<ItemHasCargo>> m_hasCargo;
	std::vector<std::unique_ptr<ItemCanBeStockPiled>> m_canBeStockPiled;
	std::vector<CraftJob*> m_craftJobForWorkPiece; // Used only for work in progress items.
	std::vector<const ItemType*> m_itemType;
	std::vector<const MaterialType*> m_materialType;
	std::vector<ItemId> m_id;
	std::vector<uint32_t> m_quality; // Always set to 0 for generic types.
	//TODO: Percent doesn't allow fine enough detail for tools wearing out over time?
	std::vector<Percent> m_percentWear; // Always set to 0 for generic types.
	std::vector<Quantity> m_quantity; // Always set to 1 for nongeneric types.
	sul::dynamic_bitset<> m_installed;
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] bool indexCanBeMoved(HasShapeIndex index) const;
public:
	Items(Area& area) : Portables(area) { }
	[[nodiscard]] Json toJson() const;
	void onChangeAmbiantSurfaceTemperature();
	ItemIndex create(ItemParamaters paramaters);
	void destroy(ItemIndex index);
	void setName(ItemIndex index, std::wstring name);
	void setLocation(ItemIndex index, BlockIndex block);
	void setLocationAndFacing(ItemIndex index, BlockIndex block, Facing facing);
	void exit(ItemIndex index);
	void pierced(ItemIndex index, Volume volume);
	void setTemperature(ItemIndex index, Temperature temperature);
	void addQuantity(ItemIndex index, Quantity delta);
	void removeQuantity(ItemIndex index, Quantity delta);
	void install(ItemIndex index, BlockIndex block, Facing facing, Faction& faction);
	void merge(ItemIndex index, ItemIndex item);
	void setOnSurface(ItemIndex index);
	void setNotOnSurface(ItemIndex index);
	void setQuality(ItemIndex index, uint32_t quality);
	void setWear(ItemIndex index, Percent wear);
	void setQuantity(ItemIndex index, Quantity quantity);
	void unsetCraftJobForWorkPiece(ItemIndex index);
	[[nodiscard]] bool isOnSurface(ItemIndex index);
	[[nodiscard]] bool isInstalled(ItemIndex index) { return m_installed[index]; }
	[[nodiscard]] Quantity getQuantity(ItemIndex index) const { return m_quantity.at(index); }
	[[nodiscard]] uint32_t getQuality(ItemIndex index) const { return m_quality.at(index); }
	[[nodiscard]] Percent getWear(ItemIndex index) const { return m_percentWear.at(index); }
	[[nodiscard]] bool isGeneric(ItemIndex index) const;
	[[nodiscard]] bool isPreparedMeal(ItemIndex index) const;
	[[nodiscard]] bool isWorkPiece(ItemIndex index) const { return m_craftJobForWorkPiece.at(index) != nullptr; }
	[[nodiscard]] CraftJob& getCraftJobForWorkPiece(ItemIndex index) const;
	[[nodiscard]] Mass getSingleUnitMass(ItemIndex index) const;
	[[nodiscard]] Mass getMass(ItemIndex index) const;
	[[nodiscard]] Volume getVolume(ItemIndex index) const;
	[[nodiscard]] const MoveType& getMoveType(ItemIndex index) const;
	[[nodiscard]] const ItemType& getItemType(ItemIndex index) const { return *m_itemType.at(index); }
	[[nodiscard]] const MaterialType& getMaterialType(ItemIndex index) const { return *m_materialType.at(index); }
	[[nodiscard]] std::unordered_set<ItemIndex> getOnSurface() { return m_onSurface; }
	[[nodiscard]] const std::unordered_set<ItemIndex> getOnSurface() const { return m_onSurface; }
	// -Cargo.
	void cargo_addActor(ItemIndex index, ActorIndex actor);
	void cargo_addItem(ItemIndex index, ItemIndex item, Quantity quantity);
	void cargo_addPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity);
	void cargo_addFluid(ItemIndex index, const FluidType& fluidType, CollisionVolume volume);
	void cargo_loadActor(ItemIndex index, ActorIndex actor);
	void cargo_loadItem(ItemIndex index, ItemIndex item, Quantity quantity);
	void cargo_loadPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity);
	void cargo_loadFluidFromLocation(ItemIndex index, const FluidType& fluidType, CollisionVolume volume, BlockIndex location);
	void cargo_loadFluidFromItem(ItemIndex index, const FluidType& fluidType, CollisionVolume volume, ItemIndex item);
	void cargo_removeActor(ItemIndex index, ActorIndex actor);
	void cargo_removeItem(ItemIndex index, ItemIndex item);
	void cargo_removeItemGeneric(ItemIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void cargo_removeFluid(ItemIndex index, CollisionVolume volume);
	void cargo_unloadActorToLocation(ItemIndex index, ActorIndex actor, BlockIndex location);
	void cargo_unloadItemToLocation(ItemIndex index, ItemIndex item, BlockIndex location);
	ItemIndex cargo_unloadGenericItemToLocation(ItemIndex index, const ItemType& itemType, const MaterialType& materialType, BlockIndex location, Quantity quantity);
	ItemIndex cargo_unloadGenericItemToLocation(ItemIndex index, ItemIndex item, BlockIndex location, Quantity quantity);
	ActorOrItemIndex cargo_unloadPolymorphicToLocation(ItemIndex index, ActorOrItemIndex actorOrItem, BlockIndex location, Quantity quantity);
	void cargo_unloadFluidToLocation(ItemIndex index, CollisionVolume volume, BlockIndex location);
	[[nodiscard]] bool cargo_exists(ItemIndex index) const;
	[[nodiscard]] bool cargo_containsActor(ItemIndex index, ActorIndex actor) const;
	[[nodiscard]] bool cargo_containsItem(ItemIndex index, ItemIndex item) const;
	[[nodiscard]] bool cargo_containsItemGeneric(ItemIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const;
	[[nodiscard]] bool cargo_containsPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity = 1) const;
	[[nodiscard]] bool cargo_containsAnyFluid(ItemIndex index) const;
	[[nodiscard]] bool cargo_containsFluidType(ItemIndex index, const FluidType& fluidType) const;
	[[nodiscard]] Volume cargo_getFluidVolume(ItemIndex index) const;
	[[nodiscard]] const FluidType& cargo_getFluidType(ItemIndex index) const;
	[[nodiscard]] bool cargo_canAddActor(ItemIndex index, ActorIndex actor) const;
	[[nodiscard]] bool cargo_canAddItem(ItemIndex index, ItemIndex item) const;
	[[nodiscard]] Mass cargo_getMass(ItemIndex index) const;
	[[nodiscard]] const std::vector<ItemIndex>& cargo_getItems(ItemIndex index) const;
	[[nodiscard]] const std::vector<ActorIndex>& cargo_getActors(ActorIndex index) const;
	// -Stockpile.
	void stockpile_maybeUnsetAndScheduleReset(ItemIndex index, Faction& faction, Step duration); 
	void stockpile_set(ItemIndex index, Faction& faction);
	void stockpile_maybeUnset(ItemIndex index, Faction& faction);
	[[nodiscard]] bool stockpile_canBeStockPiled(ItemIndex index, const Faction& faction) const;
	//TODO: Items leave area.
	// For debugging.
	void log(ItemIndex index) const;
};
