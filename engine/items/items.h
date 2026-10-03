#pragma once

#include "../dataStructures/strongVector.h"
#include "../definitions/itemType.h"
#include "../numericTypes/types.h"
#include "../portables.h"
#include "../reference.h"

class ItemHasCargo;
class ItemCanBeStockPiled;
struct CraftJob;
class Area;
class Simulation;
class EventSchedule;
class CanReserve;
struct OffsetCuboidSet;
class ConstructedShape;

struct ItemParamaters final
{
	ItemTypeId itemType;
	MaterialTypeId materialType;
	CraftJob* craftJob = nullptr;
	FactionId faction = FactionId::null();
	Point3D location = Point3D::null();
	ItemId id = ItemId::null();
	Quantity quantity = Quantity::create(1);
	Quality quality = Quality::null();
	Percent percentWear = Percent::null();
	Facing4 facing = Facing4::North;
	bool isStatic = true;
	bool installed = false;
	std::string name = "";
};
class ReMarkItemForStockPilingEvent final : public ScheduledEvent
{
	FactionId m_faction;
	ItemCanBeStockPiled* m_canBeStockPiled = nullptr;
public:
	ReMarkItemForStockPilingEvent() { assert(false); std::unreachable(); }
	ReMarkItemForStockPilingEvent(Area& area, ItemCanBeStockPiled& i, FactionId f, Step duration, Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
//TODO: Flat set.
//TODO: Change from linear to geometric delay duration.
class ItemCanBeStockPiled
{
	SmallMap<FactionId, HasScheduledEvent<ReMarkItemForStockPilingEvent>> m_scheduledEvents;
	SmallSet<FactionId> m_data;
	void scheduleReset(Area& area, FactionId faction, Step duration, Step start = Step::null());
public:
	void load(const Json& data, Area& area);
	void set(FactionId faction) { assert(!m_data.contains(faction)); m_data.insert(faction); }
	void maybeSet(FactionId faction) { m_data.insert(faction); }
	void unset(FactionId faction) { assert(m_data.contains(faction)); m_data.erase(faction); }
	void maybeUnset(FactionId faction) { m_data.erase(faction); }
	void unsetAndScheduleReset(Area& area, FactionId faction, Step duration);
	void maybeUnsetAndScheduleReset(Area& area, FactionId faction, Step duration);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool contains(FactionId faction) const { return m_data.contains(faction); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	friend class ReMarkItemForStockPilingEvent;
};
class ItemHasCargo final
{
	// Indices rather then references for speed when searching for contained items in boxes, etc.
	SmallSet<ActorIndex> m_actors;
	SmallSet<ItemIndex> m_items;
	FluidTypeId m_fluidType;
	FullDisplacement m_maxVolume;
	FullDisplacement m_volume = FullDisplacement::create(0);
	Mass m_mass = Mass::create(0);
	CollisionVolume m_fluidVolume = CollisionVolume::create(0);
public:
	ItemHasCargo(ItemTypeId itemType);
	ItemHasCargo(const Json& data);
	void addItem(Area& area, ItemIndex itemIndex);
	void addActor(Area& area, ActorIndex actorIndex);
	void addFluid(FluidTypeId fluidType, CollisionVolume volume);
	ItemIndex addItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void removeFluidVolume(FluidTypeId fluidType, CollisionVolume volume);
	void removeActor(Area& area, ActorIndex actor);
	void removeItem(Area& area, ItemIndex item);
	void removeItemGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	// When an item changes index and it has cargo update the m_carrier data of the cargo.
	void updateCarrierIndexForAllCargo(Area& area, ItemIndex newIndex);
	void moveContentsFromTo(Area& fromArea, Area& toArea, ItemIndex carrier);
	ItemIndex unloadGenericTo(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity, Point3D location);
	[[nodiscard]] const SmallSet<ActorIndex>& getActors() const { return m_actors; }
	[[nodiscard]] const SmallSet<ItemIndex>& getItems() const { return m_items; }
	[[nodiscard]] bool canAddActor(Area& area, ActorIndex index) const;
	[[nodiscard]] bool canAddItem(Area& area, ItemIndex item) const;
	[[nodiscard]] bool canAddFluid(FluidTypeId fluidType) const;
	[[nodiscard]] CollisionVolume getFluidVolume() const { return m_fluidVolume; }
	[[nodiscard]] FluidTypeId getFluidType() const { assert(m_fluidType.exists()); return m_fluidType; }
	[[nodiscard]] bool containsAnyFluid() const { return m_fluidType.exists(); }
	[[nodiscard]] bool containsFluidType(FluidTypeId fluidType) const { return m_fluidType == fluidType; }
	[[nodiscard]] bool containsActor(ActorIndex index) const { return m_actors.contains(index); }
	[[nodiscard]] bool containsItem(ItemIndex index) const { return m_items.contains(index); }
	[[nodiscard]] bool containsGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity) const;
	[[nodiscard]] bool empty() const { return m_fluidType.empty() && m_actors.empty() && m_items.empty(); }
	[[nodiscard]] Mass getMass() const { return m_mass; }
	[[nodiscard]] Json toJson() const;
	friend class Items;
};
class Items final : public Portables<Items, ItemIndex, ItemReferenceIndex, false>
{
	//TODO: change to bitset or remove.
	StrongVector<std::unique_ptr<ItemCanBeStockPiled>, ItemIndex> m_canBeStockPiled;
	StrongVector<CraftJob*, ItemIndex> m_craftJobForWorkPiece; // Used only for work in progress items.
	StrongVector<std::unique_ptr<ItemHasCargo>, ItemIndex> m_hasCargo;
	StrongVector<ItemId, ItemIndex> m_id;
	StrongBitSet<ItemIndex> m_installed;
	StrongVector<ItemTypeId, ItemIndex> m_itemType;
	StrongVector<MaterialTypeId, ItemIndex> m_solid;
	StrongVector<std::string, ItemIndex> m_name;
	//TODO: Percent doesn't allow fine enough detail for tools wearing out over time?
	StrongVector<Percent, ItemIndex> m_percentWear; // Always set to 0 for generic types.
	StrongVector<Quality, ItemIndex> m_quality; // Always set to 0 for generic types.
	StrongVector<Quantity, ItemIndex> m_quantity; // Always set to 1 for nongeneric types.
	StrongVector<ActorIndex, ItemIndex> m_pilot;
	StrongVector<std::unique_ptr<ConstructedShape>, ItemIndex> m_constructedShape;
	void moveIndex(ItemIndex oldIndex, ItemIndex newIndex);
public:
	Items(Area& area);
	void load(const Json& json);
	void loadCargoAndCraftJobs(const Json& json);
	template<typename Action>
	void forEachData(Action&& action)
	{
		forEachDataPortables(action);
		action(m_canBeStockPiled);
		action(m_craftJobForWorkPiece);
		action(m_hasCargo);
		action(m_id);
		action(m_installed);
		action(m_itemType);
		action(m_solid);
		action(m_name);
		action(m_percentWear);
		action(m_quality);
		action(m_quantity);
		action(m_onSurface);
		action(m_pilot);
		action(m_constructedShape);
	}
	// Returns index in case of nongeneric or generics with a type not present at the location.
	// If a generic of the same type is found return it instead.
	ItemIndex create(ItemParamaters paramaters);
	void remove(ItemIndex index);
	void removeAll(const SmallSet<ItemIndex>& index);
	void setName(ItemIndex index, std::string name);
	void pierced(ItemIndex index, FullDisplacement volume);
	void addQuantity(ItemIndex index, Quantity delta);
	void removeQuantity(ItemIndex index, Quantity delta, CanReserve* canReserve = nullptr);
	void install(ItemIndex index, Point3D point, Facing4 facing, FactionId faction);
	// Returns the index of the item which was passed in as 'index', it may have changed with 'item' being destroyed due to generic merge.
	ItemIndex merge(ItemIndex index, ItemIndex item);
	void setQuality(ItemIndex index, Quality quality);
	void setWear(ItemIndex index, Percent wear);
	void setQuantity(ItemIndex index, Quantity quantity);
	void unsetCraftJobForWorkPiece(ItemIndex index);
	void takeFallDamage(ItemIndex, Distance , MaterialTypeId) { /* TODO */ }
	void resetMoveType(ItemIndex index);
	// Wrap HasShapes::SetStatic and unset to support constrcuted shapes setting or unsetting Space::m_dynamic instead.
	void setStatic(ItemIndex index);
	void unsetStatic(ItemIndex index);
	void setOnSurface(ItemIndex index, bool value);
	void moveQuantity(ItemIndex index, Quantity quantity, Point3D destaination);
	ItemIndex moveTo(Items& other, ItemIndex index);
	[[nodiscard]] SmallSet<ItemIndex> getAll() const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ItemId getId(ItemIndex index) const { return m_id[index]; }
	[[nodiscard]] bool isInstalled(ItemIndex index) const { return m_installed[index]; }
	[[nodiscard]] Quantity getQuantity(ItemIndex index) const { return m_quantity[index]; }
	[[nodiscard]] Quality getQuality(ItemIndex index) const { return m_quality[index]; }
	[[nodiscard]] Percent getWear(ItemIndex index) const { return m_percentWear[index]; }
	[[nodiscard]] std::string getName(ItemIndex index) const { return m_name[index]; }
	[[nodiscard]] bool isGeneric(ItemIndex index) const;
	[[nodiscard]] bool isPreparedMeal(ItemIndex index) const;
	[[nodiscard]] bool isWorkPiece(ItemIndex index) const { return m_craftJobForWorkPiece[index] != nullptr; }
	[[nodiscard]] bool canMove(ItemIndex index) const { return pilot_exists(index) && vehicle_getSpeed(index) != 0; }
	[[nodiscard]] ConstructedShape& getConstructedShape(ItemIndex index) { return *m_constructedShape[index]; }
	[[nodiscard]] bool hasConstructedShape(ItemIndex index) { return m_constructedShape[index] != nullptr; }
	[[nodiscard]] CraftJob& getCraftJobForWorkPiece(ItemIndex index) const;
	[[nodiscard]] Mass getSingleUnitMass(ItemIndex index) const;
	[[nodiscard]] Mass getMass(ItemIndex index) const;
	[[nodiscard]] FullDisplacement getVolume(ItemIndex index) const;
	[[nodiscard]] MoveTypeId getMoveType(ItemIndex index) const;
	[[nodiscard]] ItemTypeId getItemType(ItemIndex index) const { return m_itemType[index]; }
	[[nodiscard]] MaterialTypeId getMaterialType(ItemIndex index) const { return m_solid[index]; }
	[[nodiscard]] bool canCombine(ItemIndex index, ItemIndex toMerge) const;
	[[nodiscard]] bool canMelt(ItemIndex index) const;
	[[nodiscard]] GDB_CALLABLE std::string description(ItemIndex index) const;
	// - Location.
private:
	std::pair<ItemIndex, SetLocationAndFacingResult> location_tryToSetGenericStatic(ItemIndex index, Point3D location, Facing4 facing);
	SetLocationAndFacingResult location_tryToSetNongenericStatic(ItemIndex index, Point3D location, Facing4 facing);
public:
	// Return an item index here because a static generic may combine and invalidat the passed in index.
	ItemIndex location_set(ItemIndex index, Point3D location, Facing4 facing);
	ItemIndex location_setStatic(ItemIndex index, Point3D location, Facing4 facing);
	// TODO: this shouldn't need to return anything.
	ItemIndex location_setDynamic(ItemIndex index, Point3D location, Facing4 facing);
	// Used when item already has a location, rolls back position on failure.
	std::pair<ItemIndex, SetLocationAndFacingResult> location_tryToMoveToStatic(ItemIndex index, Point3D location);
	std::pair<ItemIndex, SetLocationAndFacingResult> location_tryToMoveToDynamic(ItemIndex index, Point3D location);
	// Used when item does not have a location.
	std::pair<ItemIndex, SetLocationAndFacingResult> location_tryToSet(ItemIndex index, Point3D location, Facing4 facing);
	std::pair<ItemIndex, SetLocationAndFacingResult> location_tryToSetStatic(ItemIndex index, Point3D location, Facing4 facing);
	SetLocationAndFacingResult location_tryToSetDynamic(ItemIndex index, Point3D location, Facing4 facing);
	void location_clear(ItemIndex index);
	void location_clearStatic(ItemIndex index);
	void location_clearDynamic(ItemIndex index);
	[[nodiscard]] bool location_canEnterEverWithFacing(ItemIndex index, Point3D location, Facing4 facing) const;
	[[nodiscard]] bool location_canEnterCurrentlyWithFacing(ItemIndex index, Point3D location, Facing4 facing) const;
	[[nodiscard]] bool location_canEnterEverFrom(ItemIndex index, Point3D location, Point3D previous) const;
	[[nodiscard]] bool location_canEnterCurrentlyFrom(ItemIndex index, Point3D location, Point3D previous) const;
	// -Cargo.
	void cargo_addActor(ItemIndex index, ActorIndex actor);
	ItemIndex cargo_addItem(ItemIndex index, ItemIndex item, Quantity quantity);
	void cargo_addItemGeneric(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void cargo_addPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity);
	void cargo_addFluid(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume);
	void cargo_loadActor(ItemIndex index, ActorIndex actor);
	ItemIndex cargo_loadItem(ItemIndex index, ItemIndex item, Quantity quantity);
	ActorOrItemIndex cargo_loadPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity);
	void cargo_loadFluidFromLocation(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume, Point3D location);
	void cargo_loadFluidFromItem(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume, ItemIndex item);
	void cargo_remove(ItemIndex index, ActorOrItemIndex actorOrItem);
	void cargo_removeActor(ItemIndex index, ActorIndex actor);
	void cargo_removeItem(ItemIndex index, ItemIndex item);
	void cargo_removeItemGeneric(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void cargo_removeFluid(ItemIndex index, CollisionVolume volume);
	// TODO: Check if location can hold shape.
	void cargo_unloadActorToLocation(ItemIndex index, ActorIndex actor, Point3D location);
	void cargo_unloadItemToLocation(ItemIndex index, ItemIndex item, Point3D location);
	void cargo_updateItemIndex(ItemIndex index, ItemIndex oldIndex, ItemIndex newIndex);
	void cargo_updateActorIndex(ItemIndex index, ActorIndex oldIndex, ActorIndex newIndex);
	ItemIndex cargo_unloadGenericItemToLocation(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Point3D location, Quantity quantity);
	ItemIndex cargo_unloadGenericItemToLocation(ItemIndex index, ItemIndex item, Point3D location, Quantity quantity);
	ActorOrItemIndex cargo_unloadPolymorphicToLocation(ItemIndex index, ActorOrItemIndex actorOrItem, Point3D location, Quantity quantity);
	void cargo_unloadFluidToLocation(ItemIndex index, CollisionVolume volume, Point3D location);
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
	[[nodiscard]] const SmallSet<ItemIndex>& cargo_getItems(ItemIndex index) const;
	[[nodiscard]] const SmallSet<ActorIndex>& cargo_getActors(ItemIndex index) const;
	// Stockpile.
	void stockpile_maybeUnsetAndScheduleReset(ItemIndex index, FactionId faction, Step duration);
	void stockpile_set(ItemIndex index, FactionId faction);
	void stockpile_maybeUnset(ItemIndex index, FactionId faction);
	[[nodiscard]] bool stockpile_canBeStockPiled(ItemIndex index, FactionId faction) const;
	// Pilot.
	void pilot_set(ItemIndex item, ActorIndex pilot);
	void pilot_clear(ItemIndex item);
	[[nodiscard]] ActorIndex pilot_get(ItemIndex item) const;
	[[nodiscard]] bool pilot_exists(ItemIndex item) const { return pilot_get(item).exists(); }
	[[nodiscard]] Speed vehicle_getSpeed(ItemIndex item) const;
	[[nodiscard]] Force vehicle_getMotiveForce(ItemIndex item) const;
	// Deck.
	[[nodiscard]] const OffsetCuboidSet& getDeckOffsets(ItemIndex index) const;
	//TODO: Items leave area.
	// For debugging.
	void log(ItemIndex index) const;
	Items(Items&) = delete;
	Items(Items&&) = delete;
};
