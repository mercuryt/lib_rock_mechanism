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

struct ItemParamaters final
{
	ItemTypeId itemType;
	MaterialTypeId materialType;
	CraftJob* craftJob = nullptr;
	FactionId faction = FactionId::null();
	BlockIndex location = BlockIndex::null();
	ItemId id = ItemId::create(0);
	Quantity quantity = Quantity::create(1);
	Quality quality = Quality::create(0);
	Percent percentWear = Percent::create(0);
	Facing facing = Facing::create(0);
	bool isStatic = true;
	bool installed = false;
};
class ReMarkItemForStockPilingEvent final : public ScheduledEvent
{
	FactionId m_faction;
	ItemCanBeStockPiled& m_canBeStockPiled;
public:
	ReMarkItemForStockPilingEvent(Area& area, ItemCanBeStockPiled& i, FactionId f, Step duration, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
//TODO: Flat set.
//TODO: Change from linear to geometric delay duration.
class ItemCanBeStockPiled
{
	FactionIdMap<HasScheduledEvent<ReMarkItemForStockPilingEvent>> m_scheduledEvents;
	FactionIdSet m_data;
	void scheduleReset(Area& area, FactionId faction, Step duration, Step start = Step::null());
public:
	void load(const Json& data, Area& area);
	void set(FactionId faction) { assert(!m_data.contains(faction)); m_data.add(faction); }
	void maybeSet(FactionId faction) { m_data.add(faction); }
	void unset(FactionId faction) { assert(m_data.contains(faction)); m_data.remove(faction); }
	void maybeUnset(FactionId faction) { m_data.remove(faction); }
	void unsetAndScheduleReset(Area& area, FactionId faction, Step duration);
	void maybeUnsetAndScheduleReset(Area& area, FactionId faction, Step duration);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool contains(const FactionId faction) const { return m_data.contains(faction); }
	friend class ReMarkItemForStockPilingEvent;
};
class ItemHasCargo final
{
	ActorIndices m_actors;
	ItemIndices m_items;
	FluidTypeId m_fluidType;
	Volume m_maxVolume;
	Volume m_volume = Volume::create(0);
	Mass m_mass = Mass::create(0);
	CollisionVolume m_fluidVolume = CollisionVolume::create(0);
public:
	void addItem(Area& area, ItemIndex itemIndex);
	void addActor(Area& area, ActorIndex actorIndex);
	void addFluid(FluidTypeId fluidType, CollisionVolume volume);
	ItemIndex addItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void removeFluidVolume(FluidTypeId fluidType, CollisionVolume volume);
	void removeActor(Area& area, ActorIndex actorIndex);
	void removeItem(Area& area, ItemIndex item);
	void removeItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	// When an item changes index and it has cargo update the m_carrier data of the cargo.
	void updateCarrierIndexForAllCargo(Area& area, ItemIndex newIndex);
	ItemIndex unloadGenericTo(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity, BlockIndex location);
	[[nodiscard]] ActorIndices& getActors() { return m_actors; }
	[[nodiscard]] ItemIndices& getItems() { return m_items; }
	[[nodiscard]] const ItemIndices& getItems() const { return m_items; }
	[[nodiscard]] bool canAddActor(Area& area, ActorIndex index) const;
	[[nodiscard]] bool canAddItem(Area& area, ItemIndex item) const;
	[[nodiscard]] bool canAddFluid(FluidTypeId fluidType) const;
	[[nodiscard]] CollisionVolume getFluidVolume() const { return m_fluidVolume; }
	[[nodiscard]] FluidTypeId getFluidType() const { assert(m_fluidType.exists()); return m_fluidType; }
	[[nodiscard]] bool containsAnyFluid() const { return m_fluidType.exists(); }
	[[nodiscard]] bool containsFluidType(FluidTypeId fluidType) const { return m_fluidType == fluidType; }
	[[nodiscard]] bool containsActor(ActorIndex index) const { return std::ranges::find(m_actors, index) != m_actors.end(); }
	[[nodiscard]] bool containsItem(ItemIndex index) const { return std::ranges::find(m_items, index) != m_items.end(); }
	[[nodiscard]] bool containsGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity) const;
	[[nodiscard]] bool empty() const { return m_fluidType.exists() && m_actors.empty() && m_items.empty(); }
	[[nodiscard]] Mass getMass() const { return m_mass; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ItemHasCargo, m_actors, m_items, m_fluidType, m_volume, m_mass, m_fluidVolume);
};
class Items final : public Portables
{
	//TODO: change to bitset or remove.
	ItemIndexSet m_onSurface;
	DataVector<std::unique_ptr<ItemReferenceTarget>, ItemIndex> m_referenceTarget;
	DataVector<std::wstring, ItemIndex> m_name;
	DataVector<std::unique_ptr<ItemHasCargo>, ItemIndex> m_hasCargo;
	DataVector<std::unique_ptr<ItemCanBeStockPiled>, ItemIndex> m_canBeStockPiled;
	DataVector<CraftJob*, ItemIndex> m_craftJobForWorkPiece; // Used only for work in progress items.
	DataVector<ItemTypeId, ItemIndex> m_itemType;
	DataVector<MaterialTypeId, ItemIndex> m_materialType;
	DataVector<ItemId, ItemIndex> m_id;
	DataVector<Quality, ItemIndex> m_quality; // Always set to 0 for generic types.
	//TODO: Percent doesn't allow fine enough detail for tools wearing out over time?
	DataVector<Percent, ItemIndex> m_percentWear; // Always set to 0 for generic types.
	DataVector<Quantity, ItemIndex> m_quantity; // Always set to 1 for nongeneric types.
	DataBitSet<ItemIndex> m_installed;
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
public:
	Items(Area& area) : Portables(area) { }
	void load(const Json& json);
	void loadCargoAndCraftJobs(const Json& json, DeserializationMemo& deserializationMemo);
	void onChangeAmbiantSurfaceTemperature();
	ItemIndex create(ItemParamaters paramaters);
	void destroy(ItemIndex index);
	void setName(ItemIndex index, std::wstring name);
	void setLocation(ItemIndex index, BlockIndex block);
	void setLocationAndFacing(ItemIndex index, BlockIndex block, Facing facing);
	void exit(ItemIndex index);
	void setShape(ItemIndex index, ShapeId shape);
	void pierced(ItemIndex index, Volume volume);
	void setTemperature(ItemIndex index, Temperature temperature);
	void addQuantity(ItemIndex index, Quantity delta);
	void removeQuantity(ItemIndex index, Quantity delta);
	void install(ItemIndex index, BlockIndex block, Facing facing, FactionId faction);
	void merge(ItemIndex index, ItemIndex item);
	void setOnSurface(ItemIndex index);
	void setNotOnSurface(ItemIndex index);
	void setQuality(ItemIndex index, Quality quality);
	void setWear(ItemIndex index, Percent wear);
	void setQuantity(ItemIndex index, Quantity quantity);
	void unsetCraftJobForWorkPiece(ItemIndex index);
	[[nodiscard]] ItemIndices getAll() const;
	[[nodiscard]] ItemReference getReference(ItemIndex index) const { return *m_referenceTarget.at(index).get(); }
	[[nodiscard]] const ItemReference getReferenceConst(ItemIndex index) const { return *m_referenceTarget.at(index).get(); }
	[[nodiscard]] ItemReferenceTarget& getReferenceTarget(ItemIndex index) const { return *m_referenceTarget.at(index).get();}
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool isOnSurface(ItemIndex index);
	[[nodiscard]] bool isInstalled(ItemIndex index) { return m_installed.at(index); }
	[[nodiscard]] Quantity getQuantity(ItemIndex index) const { return m_quantity.at(index); }
	[[nodiscard]] Quality getQuality(ItemIndex index) const { return m_quality.at(index); }
	[[nodiscard]] Percent getWear(ItemIndex index) const { return m_percentWear.at(index); }
	[[nodiscard]] bool isGeneric(ItemIndex index) const;
	[[nodiscard]] bool isPreparedMeal(ItemIndex index) const;
	[[nodiscard]] bool isWorkPiece(ItemIndex index) const { return m_craftJobForWorkPiece.at(index) != nullptr; }
	[[nodiscard]] CraftJob& getCraftJobForWorkPiece(ItemIndex index) const;
	[[nodiscard]] Mass getSingleUnitMass(ItemIndex index) const;
	[[nodiscard]] Mass getMass(ItemIndex index) const;
	[[nodiscard]] Volume getVolume(ItemIndex index) const;
	[[nodiscard]] MoveTypeId getMoveType(ItemIndex index) const;
	[[nodiscard]] ItemTypeId getItemType(ItemIndex index) const { return m_itemType.at(index); }
	[[nodiscard]] MaterialTypeId getMaterialType(ItemIndex index) const { return m_materialType.at(index); }
	[[nodiscard]] auto& getOnSurface() { return m_onSurface; }
	[[nodiscard]] const auto& getOnSurface() const { return m_onSurface; }
	// -Cargo.
	void cargo_addActor(ItemIndex index, ActorIndex actor);
	void cargo_addItem(ItemIndex index, ItemIndex item, Quantity quantity);
	void cargo_addPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity);
	void cargo_addFluid(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume);
	void cargo_loadActor(ItemIndex index, ActorIndex actor);
	void cargo_loadItem(ItemIndex index, ItemIndex item, Quantity quantity);
	void cargo_loadPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity);
	void cargo_loadFluidFromLocation(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume, BlockIndex location);
	void cargo_loadFluidFromItem(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume, ItemIndex item);
	void cargo_removeActor(ItemIndex index, ActorIndex actor);
	void cargo_removeItem(ItemIndex index, ItemIndex item);
	void cargo_removeItemGeneric(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void cargo_removeFluid(ItemIndex index, CollisionVolume volume);
	// TODO: Check if location can hold shape.
	void cargo_unloadActorToLocation(ItemIndex index, ActorIndex actor, BlockIndex location);
	void cargo_unloadItemToLocation(ItemIndex index, ItemIndex item, BlockIndex location);
	void cargo_updateItemIndex(ItemIndex index, ItemIndex oldIndex, ItemIndex newIndex);
	void cargo_updateActorIndex(ItemIndex index, ActorIndex oldIndex, ActorIndex newIndex);
	ItemIndex cargo_unloadGenericItemToLocation(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, BlockIndex location, Quantity quantity);
	ItemIndex cargo_unloadGenericItemToLocation(ItemIndex index, ItemIndex item, BlockIndex location, Quantity quantity);
	ActorOrItemIndex cargo_unloadPolymorphicToLocation(ItemIndex index, ActorOrItemIndex actorOrItem, BlockIndex location, Quantity quantity);
	void cargo_unloadFluidToLocation(ItemIndex index, CollisionVolume volume, BlockIndex location);
	[[nodiscard]] bool cargo_exists(ItemIndex index) const;
	[[nodiscard]] bool cargo_containsActor(ItemIndex index, ActorIndex actor) const;
	[[nodiscard]] bool cargo_containsItem(ItemIndex index, ItemIndex item) const;
	[[nodiscard]] bool cargo_containsItemGeneric(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity) const;
	[[nodiscard]] bool cargo_containsPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity = Quantity::create(1)) const;
	[[nodiscard]] bool cargo_containsAnyFluid(ItemIndex index) const;
	[[nodiscard]] bool cargo_containsFluidType(ItemIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] CollisionVolume cargo_getFluidVolume(ItemIndex index) const;
	[[nodiscard]] FluidTypeId cargo_getFluidType(ItemIndex index) const;
	[[nodiscard]] bool cargo_canAddActor(ItemIndex index, ActorIndex actor) const;
	[[nodiscard]] bool cargo_canAddItem(ItemIndex index, ItemIndex item) const;
	[[nodiscard]] Mass cargo_getMass(ItemIndex index) const;
	[[nodiscard]] const ItemIndices& cargo_getItems(ItemIndex index) const;
	[[nodiscard]] const ActorIndices& cargo_getActors(ItemIndex index) const;
	// -Stockpile.
	void stockpile_maybeUnsetAndScheduleReset(ItemIndex index, FactionId faction, Step duration); 
	void stockpile_set(ItemIndex index, FactionId faction);
	void stockpile_maybeUnset(ItemIndex index, FactionId faction);
	[[nodiscard]] bool stockpile_canBeStockPiled(ItemIndex index, const FactionId faction) const;
	//TODO: Items leave area.
	// For debugging.
	void log(ItemIndex index) const;
	Items(Items&) = delete;
	Items(Items&&) = delete;
};
