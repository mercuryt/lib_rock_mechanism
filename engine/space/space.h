#pragma once

#include "../fire.h"
#include "../fluidType.h"
#include "../reservable.h"
#include "../area/stockpile.h"
#include "../farmFields.h"
#include "../numericTypes/types.h"
#include "../hasShapeTypes.h"
#include "../designations.h"
#include "../pointFeature.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/rtreeData.h"
#include "../dataStructures/rtreeDataIndex.h"
#include "../geometry/pointSet.h"

#include "exposedToSky.h"
#include "support.h"

#include <vector>
#include <memory>

class FluidGroup;

struct FluidData
{
	FluidTypeId type;
	FluidGroup* group;
	CollisionVolume volume;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FluidData, type, volume);
class Space
{
	// TODO: FactionId should be the outer key, not Point3D.
	SmallMap<Point3D, SmallMap<FactionId, FarmField*>> m_farmFields;
	SmallMap<Point3D, SmallMap<FactionId, PointIsPartOfStockPile>> m_stockPiles;
	constexpr static RTreeDataConfig noMerge{.splitAndMerge = false};
	SmallMap<FactionId, RTreeDataIndex<SmallSet<Project*>, uint16_t, noMerge>> m_projects;
	RTreeDataIndex<std::unique_ptr<Reservable>, uint16_t, noMerge> m_reservables;
	RTreeDataIndex<SmallMapStable<MaterialTypeId, Fire>*, uint32_t, noMerge> m_fires;
	RTreeData<MaterialTypeId> m_solid;
	RTreeDataIndex<PointFeatureSet, uint32_t> m_features;
	//TODO: store as overlaping RTree.
	RTreeDataIndex<std::vector<FluidData>, uint32_t> m_fluid;
	RTreeData<FluidTypeId> m_mist;
	RTreeData<CollisionVolume> m_totalFluidVolume;
	RTreeData<Distance> m_mistInverseDistanceFromSource;
	//TODO: store these 4 as overlaping RTree.
	RTreeDataIndex<std::vector<std::pair<ActorIndex, CollisionVolume>>, uint32_t> m_actorVolume;
	RTreeDataIndex<std::vector<std::pair<ItemIndex, CollisionVolume>>, uint32_t> m_itemVolume;
	RTreeDataIndex<SmallSet<ActorIndex>, uint32_t> m_actors;
	RTreeDataIndex<SmallSet<ItemIndex>, uint32_t> m_items;
	RTreeData<PlantIndex> m_plants;
	RTreeData<CollisionVolume> m_dynamicVolume;
	RTreeData<CollisionVolume> m_staticVolume;
	RTreeData<TemperatureDelta> m_temperatureDelta;
	std::vector<SmallSet<Offset3D>> m_indexOffsetsForNthAdjacent;
	PointsExposedToSky m_exposedToSky;
	Support m_support;
	RTreeBoolean m_visible;
	RTreeBoolean m_constructed;
	RTreeBoolean m_dynamic;
	Area& m_area;
public:
	const Coordinates m_pointToIndexConversionMultipliers;
	const Coordinates m_dimensions;
	uint m_sizeXInChunks;
	uint m_sizeXTimesYInChunks;
	//TODO: replace these with functions accessing m_dimensions.
	const Distance m_sizeX;
	const Distance m_sizeY;
	const Distance m_sizeZ;
	const DistanceWidth m_zLevelSize;
	Space(Area& area, const Distance& x, const Distance& y, const Distance& z);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void moveContentsTo(const Point3D& point, const Point3D& other);
	void maybeContentsFalls(const Point3D& point);
	void setDynamic(const Point3D& point);
	void unsetDynamic(const Point3D& point);
	void doSupportStep() { m_support.doStep(m_area); }
	[[nodiscard]] uint size() const { return m_dimensions.prod(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] Point3D getCenterAtGroundLevel() const;
	// TODO: Return limited set.
	[[nodiscard]] SmallSet<Point3D> getAdjacentWithEdgeAndCornerAdjacent(const Point3D& point) const;
	[[nodiscard]] SmallSet<Point3D> getDirectlyAdjacent(const Point3D& point) const;
	[[nodiscard]] SmallSet<Point3D> getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(const Point3D& point) const;
	[[nodiscard]] SmallSet<Point3D> getAdjacentOnSameZLevelOnly(const Point3D& point) const;
	// getNthAdjacent is not const because the point offsets are created and cached.
	[[nodiscard]] SmallSet<Point3D> getNthAdjacent(const Point3D& point, const Distance& distance);
	[[nodiscard]] bool isAdjacentToActor(const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItem(const Point3D& point, const ItemIndex& item) const;
	[[nodiscard]] bool isConstructed(const Point3D& point) const { return m_constructed.query(point); }
	[[nodiscard]] bool isDynamic(const Point3D& point) const { return m_dynamic.query(point); }
	[[nodiscard]] bool canSeeIntoFromAlways(const Point3D& point, const Point3D& other) const;
	[[nodiscard]] bool isVisible(const Point3D& point) const { return m_visible.query(point); }
	[[nodiscard]] bool canSeeThrough(const Point3D& point) const;
	[[nodiscard]] bool canSeeThroughFloor(const Point3D& point) const;
	[[nodiscard]] bool canSeeThroughFrom(const Point3D& point, const Point3D& other) const;
	[[nodiscard]] bool isSupport(const Point3D& point) const;
	[[nodiscard]] bool isExposedToSky(const Point3D& point) const;
	[[nodiscard]] bool isEdge(const Point3D& point) const;
	[[nodiscard]] bool hasLineOfSightTo(const Point3D& point, const Point3D& other) const;
	[[nodiscard]] Cuboid getZLevel(const Distance& z);
	[[nodiscard]] const Support& getSupport() const { return m_support; }
	[[nodiscard]] Support& getSupport() { return m_support; }
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(const Point3D& point, bool exposed);
	void setBelowVisible(const Point3D& point);
	//TODO: Use std::function instead of template.
	template <typename F>
	[[nodiscard]] SmallSet<Point3D> collectAdjacentsWithCondition(const Point3D& point, F&& condition)
	{
		SmallSet<Point3D> output;
		std::stack<Point3D> openList;
		openList.push(point);
		output.insert(point);
		while(!openList.empty())
		{
			Point3D point = openList.top();
			openList.pop();
			for(const Point3D& adjacent : getDirectlyAdjacent(point))
				if(condition(adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	template <typename F>
	[[nodiscard]] Point3D getPointInRangeWithCondition(const Point3D& point, const Distance& range, F&& condition)
	{
		std::stack<Point3D> open;
		open.push(point);
		SmallSet<Point3D> closed;
		while(!open.empty())
		{
			Point3D point = open.top();
			assert(point.exists());
			if(condition(point))
				return point;
			open.pop();
			for(const Point3D& adjacent : getDirectlyAdjacent(point))
				if(point.taxiDistanceTo(adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
		}
		return Point3D::null();
	}
	[[nodiscard]] SmallSet<Point3D> collectAdjacentsInRange(const Point3D& point, const Distance& range);
	// -Designation
	[[nodiscard]] bool designation_has(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation) const;
	void designation_set(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation);
	void designation_unset(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation);
	void designation_maybeUnset(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation);
	// -Solid.
private:
	void solid_setShared(const Point3D& point, const MaterialTypeId& materialType, bool wasEmpty);
public:
	void solid_set(const Point3D& point, const MaterialTypeId& materialType, bool constructed);
	void solid_setNot(const Point3D& point);
	void solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed);
	void solid_setNotCuboid(const Cuboid& cuboid);
	void solid_setDynamic(const Point3D& point, const MaterialTypeId& materialType, bool constructed);
	void solid_setNotDynamic(const Point3D& point);
	[[nodiscard]] bool solid_is(const Point3D& point) const;
	[[nodiscard]] MaterialTypeId solid_get(const Point3D& point) const;
	[[nodiscard]] Mass solid_getMass(const Point3D& point) const;
	[[nodiscard]] Mass solid_getMass(const CuboidSet& cuboidSet) const;
	[[nodiscard]] MaterialTypeId solid_getHardest(const SmallSet<Point3D>& space);
	// -PointFeature.
	// TODO: make construct / hew / remove work with cuboids.
	void pointFeature_construct(const Point3D& point, const PointFeatureTypeId& featureType, const MaterialTypeId& materialType);
	void pointFeature_hew(const Point3D& point, const PointFeatureTypeId& featureType);
	void pointFeature_remove(const Point3D& point, const PointFeatureTypeId& type);
	void pointFeature_removeAll(const Point3D& point);
	void pointFeature_lock(const Point3D& point, const PointFeatureTypeId& type);
	void pointFeature_unlock(const Point3D& point, const PointFeatureTypeId& type);
	void pointFeature_close(const Point3D& point, const PointFeatureTypeId& type);
	void pointFeature_open(const Point3D& point, const PointFeatureTypeId& type);
	void pointFeature_setTemperature(const Point3D& point, const Temperature& temperature);
	void pointFeature_setAll(const Point3D& point, PointFeatureSet& features);
	[[nodiscard]] const PointFeature* pointFeature_at(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const;
	[[nodiscard]] bool pointFeature_empty(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_blocksEntrance(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canStandAbove(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canStandIn(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_isSupport(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canEnterFromBelow(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canEnterFromAbove(const Point3D& point, const Point3D& from) const;
	[[nodiscard]] bool pointFeature_multiTileCanEnterAtNonZeroZOffset(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_isOpaque(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_floorIsOpaque(const Point3D& point) const;
	[[nodiscard]] MaterialTypeId pointFeature_getMaterialType(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_contains(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const;
	[[nodiscard]] const PointFeatureSet& pointFeature_getAll(const Point3D& point) const;
	// -Fluids
	void fluid_spawnMist(const Point3D& point, const FluidTypeId& fluidType, const Distance maxMistSpread = Distance::create(0));
	void fluid_clearMist(const Point3D& point);
	Distance fluid_getMistInverseDistanceToSource(const Point3D& point) const;
	FluidGroup* fluid_getGroup(const Point3D& point, const FluidTypeId& fluidType) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void fluid_add(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_add(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType);
	// To be used durring read step.
	void fluid_remove(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType);
	// To be used used durring write step.
	void fluid_removeSyncronus(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_removeAllSyncronus(const Point3D& point);
	// Move less dense fluids to their group's excessVolume until Config::maxPointVolume is achieved.
	void fluid_resolveOverfull(const Point3D& point);
	void fluid_onPointSetSolid(const Point3D& point);
	void fluid_onPointSetNotSolid(const Point3D& point);
	void fluid_mistSetFluidTypeAndInverseDistance(const Point3D& point, const FluidTypeId& fluidType, const Distance& inverseDistance);
	// TODO: This could probably be resolved in a better way.
	// Exposing these two methods breaks encapusalition a bit but allows better performance from fluidGroup.
	void fluid_setAllUnstableExcept(const Point3D& point, const FluidTypeId& fluidType);
	// To be used by DrainQueue / FillQueue.
	void fluid_drainInternal(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_fillInternal(const Point3D& point, const CollisionVolume& volume, FluidGroup& fluidGroup);
	// To be used by fluid group split, so the space which will be in soon to be created groups can briefly have fluid without having a fluidGroup.
	// Fluid group will be set again in addPoint within the new group's constructor.
	// This prevents a problem where addPoint attempts to remove a point from a group which it has already been removed from.
	// TODO: Seems messy.
	void fluid_unsetGroupInternal(const Point3D& point, const FluidTypeId& fluidType);
	void fluid_setGroupInternal(const Point3D& point, const FluidTypeId& fluidType, FluidGroup& fluidGroup);
	void fluid_maybeRecordFluidOnDeck(const Point3D& point);
	void fluid_maybeEraseFluidOnDeck(const Point3D& point);
	// To be called from FluidGroup::splitStep only.
	[[nodiscard]] bool fluid_undisolveInternal(const Point3D& point, FluidGroup& fluidGroup);
private:void fluid_destroyData(const Point3D& point, const FluidTypeId& fluidType);
	void fluid_setTotalVolume(const Point3D& point, const CollisionVolume& volume);
public: [[nodiscard]] bool fluid_canEnterCurrently(const Point3D& point, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool fluid_isAdjacentToGroup(const Point3D& point, const FluidGroup& fluidGroup) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeCanEnter(const Point3D& point, const FluidTypeId& fluidType) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeContains(const Point3D& point, const FluidTypeId& fluidType) const;
	[[nodiscard]] FluidTypeId fluid_getTypeWithMostVolume(const Point3D& point) const;
	[[nodiscard]] bool fluid_canEnterEver(const Point3D& point) const;
	[[nodiscard]] bool fluid_typeCanEnterCurrently(const Point3D& point, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool fluid_any(const Point3D& point) const;
	[[nodiscard]] bool fluid_contains(const Point3D& point, const FluidTypeId& fluidType) const;
	[[nodiscard]] const std::vector<FluidData>& fluid_getAll(const Point3D& point) const;
	[[nodiscard]] const std::vector<FluidData> fluid_getAllSortedByDensityAscending(const Point3D& point);
	[[nodiscard]] CollisionVolume fluid_getTotalVolume(const Point3D& point) const;
	[[nodiscard]] FluidTypeId fluid_getMist(const Point3D& point) const;
	[[nodiscard]] const FluidData* fluid_getData(const Point3D& point, const FluidTypeId& fluidType) const;
	// Floating
	void floating_maybeSink(const Point3D& point);
	void floating_maybeFloatUp(const Point3D& point);
	// -Fire
	void fire_maybeIgnite(const Point3D& point, const MaterialTypeId& materialType);
	void fire_setPointer(const Point3D& point, SmallMapStable<MaterialTypeId, Fire>* pointer);
	void fire_clearPointer(const Point3D& point);
	[[nodiscard]] bool fire_exists(const Point3D& point) const;
	[[nodiscard]] bool fire_existsForMaterialType(const Point3D& point, const MaterialTypeId& materialType) const;
	[[nodiscard]] FireStage fire_getStage(const Point3D& point) const;
	[[nodiscard]] Fire& fire_get(const Point3D& point, const MaterialTypeId& materialType);
	// -Reservations
	void reserve(const Point3D& point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback = nullptr);
	void unreserve(const Point3D& point, CanReserve& canReserve);
	void dishonorAllReservations(const Point3D& point);
	void setReservationDishonorCallback(const Point3D& point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	[[nodiscard]] bool isReserved(const Point3D& point, const FactionId& faction) const;
	// To be used by CanReserve::translateAndReservePositions.
	[[nodiscard]] Reservable& getReservable(const Point3D& point);
	// -Actors
	void actor_recordStatic(const Point3D& point, const ActorIndex& actor, const CollisionVolume& volume);
	void actor_recordDynamic(const Point3D& point, const ActorIndex& actor, const CollisionVolume& volume);
	void actor_eraseStatic(const Point3D& point, const ActorIndex& actor);
	void actor_eraseDynamic(const Point3D& point, const ActorIndex& actor);
	void actor_setTemperature(const Point3D& point, const Temperature& temperature);
	void actor_updateIndex(const Point3D& point, const ActorIndex& oldIndex, const ActorIndex& newIndex);
	[[nodiscard]] bool actor_canStandIn(const Point3D& point) const;
	[[nodiscard]] bool actor_contains(const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool actor_empty(const Point3D& point) const;
	[[nodiscard]] FullDisplacement actor_volumeOf(const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] const SmallSet<ActorIndex>& actor_getAll(const Point3D& point) const;
	// -Items
	void item_record(const Point3D& point, const ItemIndex& item, const CollisionVolume& volume);
	void item_recordStatic(const Point3D& point, const ItemIndex& item, const CollisionVolume& volume);
	void item_recordDynamic(const Point3D& point, const ItemIndex& item, const CollisionVolume& volume);
	void item_erase(const Point3D& point, const ItemIndex& item);
	void item_eraseDynamic(const Point3D& point, const ItemIndex& item);
	void item_eraseStatic(const Point3D& point, const ItemIndex& item);
	void item_setTemperature(const Point3D& point, const Temperature& temperature);
	void item_disperseAll(const Point3D& point);
	void item_updateIndex(const Point3D& point, const ItemIndex& oldIndex, const ItemIndex& newIndex);
	ItemIndex item_addGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	//ItemIndex get(const Point3D& point, ItemType& itemType) const;
	[[nodiscard]] Quantity item_getCount(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] ItemIndex item_getGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] const SmallSet<ItemIndex> item_getAll(const Point3D& point) const;
	[[nodiscard]] bool item_hasInstalledType(const Point3D& point, const ItemTypeId& itemType) const;
	[[nodiscard]] bool item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool item_hasContainerContainingFluidTypeCarryableBy(const Point3D& point, const ActorIndex& actor, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool item_empty(const Point3D& point) const;
	[[nodiscard]] bool item_contains(const Point3D& point, const ItemIndex& item) const;
	// -Plant
	PlantIndex plant_create(const Point3D& point, const PlantSpeciesId& plantSpecies, Percent growthPercent = Percent::null());
	void plant_updateGrowingStatus(const Point3D& point);
	void plant_setTemperature(const Point3D& point, const Temperature& temperature);
	void plant_erase(const Point3D& point);
	void plant_set(const Point3D& point, const PlantIndex& plant);
	PlantIndex plant_get(const Point3D& point) const;
	bool plant_canGrowHereCurrently(const Point3D& point, const PlantSpeciesId& plantSpecies) const;
	bool plant_canGrowHereAtSomePointToday(const Point3D& point, const PlantSpeciesId& plantSpecies) const;
	bool plant_canGrowHereEver(const Point3D& point, const PlantSpeciesId& plantSpecies) const;
	bool plant_anythingCanGrowHereEver(const Point3D& point) const;
	bool plant_exists(const Point3D& point) const;
	// -Shape / Move
	void shape_addStaticVolume(const Point3D& point, const CollisionVolume& volume);
	void shape_removeStaticVolume(const Point3D& point, const CollisionVolume& volume);
	void shape_addDynamicVolume(const Point3D& point, const CollisionVolume& volume);
	void shape_removeDynamicVolume(const Point3D& point, const CollisionVolume& volume);
	[[nodiscard]] bool shape_anythingCanEnterEver(const Point3D& point) const;
	[[nodiscard]] bool shape_anythingCanEnterCurrently(const Point3D& point) const;
	[[nodiscard]] bool shape_canFitEverOrCurrentlyDynamic(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_canFitEverOrCurrentlyStatic(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_canFitEver(const Point3D& point, const ShapeId& shape, const Facing4& facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverFrom(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType) const;
	[[nodiscard]] Facing4 shape_canEnterEverWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType) const;
	// CanEnterCurrently methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyFrom(const Point3D& point, const ShapeId& shape, const Point3D& other, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_moveTypeCanEnter(const Point3D& point, const MoveTypeId& moveType) const;
	[[nodiscard]] bool shape_moveTypeCanEnterFrom(const Point3D& point, const MoveTypeId& moveType, const Point3D& from) const;
	[[nodiscard]] bool shape_moveTypeCanBreath(const Point3D& point, const MoveTypeId& moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithFacing(const Point3D& point, const ShapeId& Shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] std::pair<bool, Facing4> shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithAnyFacing(const Point3D& point, const ShapeId& shape, const OccupiedSpaceForHasShape& occupied) const;
	[[nodiscard]] MoveCost shape_moveCostFrom(const Point3D& point, const MoveTypeId& moveType, const Point3D& from) const;
	[[nodiscard]] bool shape_canStandIn(const Point3D& point) const;
	[[nodiscard]] CollisionVolume shape_getDynamicVolume(const Point3D& point) const;
	[[nodiscard]] CollisionVolume shape_getStaticVolume(const Point3D& point) const;
	[[nodiscard]] Quantity shape_getQuantityOfItemWhichCouldFit(const Point3D& point, const ItemTypeId& itemType) const;
	[[nodiscard]] SmallSet<Point3D> shape_getBelowPointsWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing) const;
	[[nodiscard]] std::pair<Point3D, Facing4> shape_getNearestEnterableEverPointWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType);
	[[nodiscard]] std::pair<Point3D, Facing4> shape_getNearestEnterableEverOrCurrentlyPointWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType);
	// -FarmField
	void farm_insert(const Point3D& point, const FactionId& faction, FarmField& farmField);
	void farm_remove(const Point3D& point, const FactionId& faction);
	void farm_designateForHarvestIfPartOfFarmField(const Point3D& point, const PlantIndex& plant);
	void farm_designateForGiveFluidIfPartOfFarmField(const Point3D& point, const PlantIndex& plant);
	void farm_maybeDesignateForSowingIfPartOfFarmField(const Point3D& point);
	void farm_removeAllHarvestDesignations(const Point3D& point);
	void farm_removeAllGiveFluidDesignations(const Point3D& point);
	void farm_removeAllSowSeedsDesignations(const Point3D& point);
	[[nodiscard]] bool farm_isSowingSeasonFor(const PlantSpeciesId& species) const;
	[[nodiscard]] bool farm_contains(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] FarmField* farm_get(const Point3D& point, const FactionId& faction);
	[[nodiscard]] const FarmField* farm_get(const Point3D& point, const FactionId& faction) const;
	// -StockPile
	void stockpile_recordMembership(const Point3D& point, StockPile& stockPile);
	void stockpile_recordNoLongerMember(const Point3D& point, StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void stockpile_updateActive(const Point3D& point);
	[[nodiscard]] StockPile* stockpile_getForFaction(const Point3D& point, const FactionId& faction);
	[[nodiscard]] const StockPile* stockpile_getForFaction(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] bool stockpile_contains(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] bool stockpile_isAvalible(const Point3D& point, const FactionId& faction) const;
	// -Project
	void project_add(const Point3D& point, Project& project);
	void project_remove(const Point3D& point, Project& project);
	Percent project_getPercentComplete(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] Project* project_get(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] Project* project_getIfBegun(const Point3D& point, const FactionId& faction) const;
	// -Temperature
	void temperature_freeze(const Point3D& point, const FluidTypeId& fluidType);
	void temperature_melt(const Point3D& point);
	void temperature_apply(const Point3D& point, const Temperature& temperature, const TemperatureDelta& delta);
	void temperature_updateDelta(const Point3D& point, const TemperatureDelta& delta);
	const Temperature& temperature_getAmbient(const Point3D& point) const;
	Temperature temperature_getDailyAverageAmbient(const Point3D& point) const;
	Temperature temperature_get(const Point3D& point) const;
	bool temperature_transmits(const Point3D& point) const;
	[[nodiscard]] std::string toString(const Point3D& point) const;
	Space(Space&) = delete;
	Space(Space&&) = delete;
};