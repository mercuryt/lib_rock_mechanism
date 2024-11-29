#pragma once

#include "portables.h"
#include "types.h"
#include "itemType.h"
#include "reference.h"

class ItemHasCargo;
class ItemCanBeStockPiled;
struct CraftJob;
class Area;
class Simulation;
class EventSchedule;
class CanReserve;

struct ItemParamaters final
{
	ItemTypeId itemType;
	MaterialTypeId materialType;
	CraftJob* craftJob = nullptr;
	FactionId faction = FactionId::null();
	BlockIndex location = BlockIndex::null();
	ItemId id = ItemId::null();
	Quantity quantity = Quantity::create(1);
	Quality quality = Quality::null();
	Percent percentWear = Percent::null();
	Facing facing = Facing::create(0);
	bool isStatic = true;
	bool installed = false;
	std::wstring name = L"";
};
class ReMarkItemForStockPilingEvent final : public ScheduledEvent
{
	FactionId m_faction;
	ItemCanBeStockPiled& m_canBeStockPiled;
public:
	ReMarkItemForStockPilingEvent(Area& area, ItemCanBeStockPiled& i, const FactionId& f, const Step& duration, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
//TODO: Flat set.
//TODO: Change from linear to geometric delay duration.
class ItemCanBeStockPiled
{
	FactionIdMap<HasScheduledEvent<ReMarkItemForStockPilingEvent>> m_scheduledEvents;
	FactionIdSet m_data;
	void scheduleReset(Area& area, const FactionId& faction, const Step& duration, Step start = Step::null());
public:
	void load(const Json& data, Area& area);
	void set(const FactionId& faction) { assert(!m_data.contains(faction)); m_data.add(faction); }
	void maybeSet(const FactionId& faction) { m_data.add(faction); }
	void unset(const FactionId& faction) { assert(m_data.contains(faction)); m_data.remove(faction); }
	void maybeUnset(const FactionId& faction) { m_data.remove(faction); }
	void unsetAndScheduleReset(Area& area, const FactionId& faction, const Step& duration);
	void maybeUnsetAndScheduleReset(Area& area, const FactionId& faction, const Step& duration);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool contains(const FactionId& faction) const { return m_data.contains(faction); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	friend class ReMarkItemForStockPilingEvent;
};
class ItemHasCargo final
{
	// Indices rather then references for speed when searching for contained items in boxes, etc.
	ActorIndices m_actors;
	ItemIndices m_items;
	FluidTypeId m_fluidType;
	Volume m_maxVolume;
	Volume m_volume = Volume::create(0);
	Mass m_mass = Mass::create(0);
	CollisionVolume m_fluidVolume = CollisionVolume::create(0);
public:
	ItemHasCargo(const ItemTypeId& itemType) : m_maxVolume(ItemType::getInternalVolume(itemType)) { }
	ItemHasCargo(const Json& data);
	void addItem(Area& area, const ItemIndex& itemIndex);
	void addActor(Area& area, const ActorIndex& actorIndex);
	void addFluid(const FluidTypeId& fluidType, const CollisionVolume& volume);
	ItemIndex addItemGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	void removeFluidVolume(const FluidTypeId& fluidType, const CollisionVolume& volume);
	void removeActor(Area& area, const ActorIndex& actorIndex);
	void removeItem(Area& area, const ItemIndex& item);
	void removeItemGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	// When an item changes index and it has cargo update the m_carrier data of the cargo.
	void updateCarrierIndexForAllCargo(Area& area, const ItemIndex& newIndex);
	ItemIndex unloadGenericTo(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity, const BlockIndex& location);
	[[nodiscard]] const ActorIndices& getActors() const { return m_actors; }
	[[nodiscard]] const ItemIndices& getItems() const { return m_items; }
	[[nodiscard]] bool canAddActor(Area& area, const ActorIndex& index) const;
	[[nodiscard]] bool canAddItem(Area& area, const ItemIndex& item) const;
	[[nodiscard]] bool canAddFluid(const FluidTypeId& fluidType) const;
	[[nodiscard]] CollisionVolume getFluidVolume() const { return m_fluidVolume; }
	[[nodiscard]] FluidTypeId getFluidType() const { assert(m_fluidType.exists()); return m_fluidType; }
	[[nodiscard]] bool containsAnyFluid() const { return m_fluidType.exists(); }
	[[nodiscard]] bool containsFluidType(const FluidTypeId& fluidType) const { return m_fluidType == fluidType; }
	[[nodiscard]] bool containsActor(const ActorIndex& index) const { return std::ranges::find(m_actors, index) != m_actors.end(); }
	[[nodiscard]] bool containsItem(const ItemIndex& index) const { return std::ranges::find(m_items, index) != m_items.end(); }
	[[nodiscard]] bool containsGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity) const;
	[[nodiscard]] bool empty() const { return m_fluidType.empty() && m_actors.empty() && m_items.empty(); }
	[[nodiscard]] Mass getMass() const { return m_mass; }
	[[nodiscard]] Json toJson() const;
	friend class Items;
};
class Items final : public Portables<Items, ItemIndex, ItemReferenceIndex>
{
	//TODO: change to bitset or remove.
	ItemIndexSet m_onSurface;
	DataVector<std::unique_ptr<ItemCanBeStockPiled>, ItemIndex> m_canBeStockPiled;
	DataVector<CraftJob*, ItemIndex> m_craftJobForWorkPiece; // Used only for work in progress items.
	DataVector<std::unique_ptr<ItemHasCargo>, ItemIndex> m_hasCargo;
	DataVector<ItemId, ItemIndex> m_id;
	DataBitSet<ItemIndex> m_installed;
	DataVector<ItemTypeId, ItemIndex> m_itemType;
	DataVector<MaterialTypeId, ItemIndex> m_materialType;
	DataVector<std::wstring, ItemIndex> m_name;
	//TODO: Percent doesn't allow fine enough detail for tools wearing out over time?
	DataVector<Percent, ItemIndex> m_percentWear; // Always set to 0 for generic types.
	DataVector<Quality, ItemIndex> m_quality; // Always set to 0 for generic types.
	DataVector<Quantity, ItemIndex> m_quantity; // Always set to 1 for nongeneric types.
	void resize(const ItemIndex& newSize);
	void moveIndex(const ItemIndex& oldIndex, const ItemIndex& newIndex);
public:
	Items(Area& area) : Portables(area, false) { }
	void load(const Json& json);
	void loadCargoAndCraftJobs(const Json& json);
	void onChangeAmbiantSurfaceTemperature();
	ItemIndex create(ItemParamaters paramaters);
	void destroy(const ItemIndex& index);
	void setName(const ItemIndex& index, std::wstring name);
	// Returns index in case of nongeneric or generics with a type not present at the location.
	// If a generic of the same type is found return it instead.
	ItemIndex setLocation(const ItemIndex& index, const BlockIndex& block);
	ItemIndex setLocationAndFacing(const ItemIndex& index, const BlockIndex& block, const Facing facing);
	void exit(const ItemIndex& index);
	void setShape(const ItemIndex& index, const ShapeId shape);
	void pierced(const ItemIndex& index, const Volume volume);
	void setTemperature(const ItemIndex& index, const Temperature& temperature);
	void addQuantity(const ItemIndex& index, const Quantity& delta);
	void removeQuantity(const ItemIndex& index, const Quantity& delta, CanReserve* canReserve = nullptr);
	void install(const ItemIndex& index, const BlockIndex& block, const Facing& facing, const FactionId& faction);
	// Returns the index of the item which was passed in as 'index', it may have changed with 'item' being destroyed
	ItemIndex merge(const ItemIndex& index, const ItemIndex& item);
	void setOnSurface(const ItemIndex& index);
	void setNotOnSurface(const ItemIndex& index);
	void setQuality(const ItemIndex& index, const Quality& quality);
	void setWear(const ItemIndex& index, const Percent& wear);
	void setQuantity(const ItemIndex& index, const Quantity& quantity);
	void unsetCraftJobForWorkPiece(const ItemIndex& index);
	[[nodiscard]] ItemIndices getAll() const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool isOnSurface(const ItemIndex& index);
	[[nodiscard]] bool isInstalled(const ItemIndex& index) { return m_installed[index]; }
	[[nodiscard]] Quantity getQuantity(const ItemIndex& index) const { return m_quantity[index]; }
	[[nodiscard]] Quality getQuality(const ItemIndex& index) const { return m_quality[index]; }
	[[nodiscard]] Percent getWear(const ItemIndex& index) const { return m_percentWear[index]; }
	[[nodiscard]] bool isGeneric(const ItemIndex& index) const;
	[[nodiscard]] bool isPreparedMeal(const ItemIndex& index) const;
	[[nodiscard]] bool isWorkPiece(const ItemIndex& index) const { return m_craftJobForWorkPiece[index] != nullptr; }
	[[nodiscard]] CraftJob& getCraftJobForWorkPiece(const ItemIndex& index) const;
	[[nodiscard]] Mass getSingleUnitMass(const ItemIndex& index) const;
	[[nodiscard]] Mass getMass(const ItemIndex& index) const;
	[[nodiscard]] Volume getVolume(const ItemIndex& index) const;
	[[nodiscard]] MoveTypeId getMoveType(const ItemIndex& index) const;
	[[nodiscard]] ItemTypeId getItemType(const ItemIndex& index) const { return m_itemType[index]; }
	[[nodiscard]] MaterialTypeId getMaterialType(const ItemIndex& index) const { return m_materialType[index]; }
	[[nodiscard]] auto& getOnSurface() { return m_onSurface; }
	[[nodiscard]] const auto& getOnSurface() const { return m_onSurface; }
	// -Cargo.
	void cargo_addActor(const ItemIndex& index, const ActorIndex& actor);
	void cargo_addItem(const ItemIndex& index, const ItemIndex& item, const Quantity& quantity);
	void cargo_addItemGeneric(const ItemIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	void cargo_addPolymorphic(const ItemIndex& index, const ActorOrItemIndex& actorOrItemIndex, const Quantity& quantity);
	void cargo_addFluid(const ItemIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume);
	void cargo_loadActor(const ItemIndex& index, const ActorIndex& actor);
	void cargo_loadItem(const ItemIndex& index, const ItemIndex& item, const Quantity& quantity);
	void cargo_loadPolymorphic(const ItemIndex& index, const ActorOrItemIndex& actorOrItem, const Quantity& quantity);
	void cargo_loadFluidFromLocation(const ItemIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume, const BlockIndex& location);
	void cargo_loadFluidFromItem(const ItemIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume, const ItemIndex& item);
	void cargo_removeActor(const ItemIndex& index, const ActorIndex& actor);
	void cargo_removeItem(const ItemIndex& index, const ItemIndex& item);
	void cargo_removeItemGeneric(const ItemIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	void cargo_removeFluid(const ItemIndex& index, const CollisionVolume& volume);
	// TODO: Check if location can hold shape.
	void cargo_unloadActorToLocation(const ItemIndex& index, const ActorIndex& actor, const BlockIndex& location);
	void cargo_unloadItemToLocation(const ItemIndex& index, const ItemIndex& item, const BlockIndex& location);
	void cargo_updateItemIndex(const ItemIndex& index, const ItemIndex& oldIndex, const ItemIndex& newIndex);
	void cargo_updateActorIndex(const ItemIndex& index, const ActorIndex& oldIndex, const ActorIndex& newIndex);
	ItemIndex cargo_unloadGenericItemToLocation(const ItemIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const BlockIndex& location, const Quantity& quantity);
	ItemIndex cargo_unloadGenericItemToLocation(const ItemIndex& index, const ItemIndex& item, const BlockIndex& location, const Quantity& quantity);
	ActorOrItemIndex cargo_unloadPolymorphicToLocation(const ItemIndex& index, const ActorOrItemIndex& actorOrItem, const BlockIndex& location, const Quantity& quantity);
	void cargo_unloadFluidToLocation(const ItemIndex& index, const CollisionVolume& volume, const BlockIndex& location);
	[[nodiscard]] bool cargo_exists(const ItemIndex& index) const;
	[[nodiscard]] bool cargo_containsActor(const ItemIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool cargo_containsItem(const ItemIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool cargo_containsItemGeneric(const ItemIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity) const;
	[[nodiscard]] bool cargo_containsPolymorphic(const ItemIndex& index, const ActorOrItemIndex& actorOrItem, const Quantity quantity = Quantity::create(1)) const;
	[[nodiscard]] bool cargo_containsAnyFluid(const ItemIndex& index) const;
	[[nodiscard]] bool cargo_containsFluidType(const ItemIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] CollisionVolume cargo_getFluidVolume(const ItemIndex& index) const;
	[[nodiscard]] FluidTypeId cargo_getFluidType(const ItemIndex& index) const;
	[[nodiscard]] bool cargo_canAddActor(const ItemIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool cargo_canAddItem(const ItemIndex& index, const ItemIndex& item) const;
	[[nodiscard]] Mass cargo_getMass(const ItemIndex& index) const;
	[[nodiscard]] const ItemIndices& cargo_getItems(const ItemIndex& index) const;
	[[nodiscard]] const ActorIndices& cargo_getActors(const ItemIndex& index) const;
	// -Stockpile.
	void stockpile_maybeUnsetAndScheduleReset(const ItemIndex& index, const FactionId& faction, const Step& duration); 
	void stockpile_set(const ItemIndex& index, const FactionId& faction);
	void stockpile_maybeUnset(const ItemIndex& index, const FactionId& faction);
	[[nodiscard]] bool stockpile_canBeStockPiled(const ItemIndex& index, const FactionId& faction) const;
	//TODO: Items leave area.
	// For debugging.
	void log(const ItemIndex& index) const;
	Items(Items&) = delete;
	Items(Items&&) = delete;
};
