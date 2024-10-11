#pragma once

#include "../fire.h"
#include "../fluidType.h"
#include "../reservable.h"
#include "../stockpile.h"
#include "../farmFields.h"
#include "../types.h"
#include "../designations.h"
#include "../blockFeature.h"
#include "../index.h"

#include "blockIndexArray.h"
#include "dataVector.h"
#include "index.h"

#include <vector>
#include <memory>

class FluidGroup;
class LocationBucket;

struct FluidData
{
	FluidTypeId type;
	FluidGroup* group;
	CollisionVolume volume;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FluidData, type, volume);
/*
struct BlockIsPartOfStockPile
{
	StockPile& stockPile;
	bool active;
};
*/
//TODO: Upgrade these vectors to arrays.
using ActorIndicesForBlock = ActorIndices; //ActorIndicesArray<Config::maxActorsPerBlock>;
using ItemIndicesForBlock = ItemIndices; //ItemIndicesArray<Config::maxItemsPerBlock>;
class Blocks
{
	std::array<int32_t, 26> m_offsetsForAdjacentCountTable;
	BlockIndexMap<FactionIdMap<FarmField*>> m_farmFields;
	BlockIndexMap<FactionIdMap<BlockIsPartOfStockPile>> m_stockPiles;
	DataVector<std::unique_ptr<Reservable>, BlockIndex> m_reservables;
	DataVector<MaterialTypeId, BlockIndex> m_materialType;
	DataVector<std::vector<BlockFeature>, BlockIndex> m_features;
	DataVector<std::vector<FluidData>, BlockIndex> m_fluid;
	DataVector<FluidTypeId, BlockIndex> m_mist;
	DataVector<CollisionVolume, BlockIndex> m_totalFluidVolume;
	DataVector<DistanceInBlocks, BlockIndex> m_mistInverseDistanceFromSource;
	DataVector<std::vector<std::pair<ActorIndex, CollisionVolume>>, BlockIndex> m_actorVolume;
	DataVector<std::vector<std::pair<ItemIndex, CollisionVolume>>, BlockIndex> m_itemVolume;
	DataVector<ActorIndicesForBlock, BlockIndex> m_actors;
	DataVector<ItemIndicesForBlock, BlockIndex> m_items;
	DataBitSet<BlockIndex> m_hasActors;
	DataBitSet<BlockIndex> m_hasItems;
	DataVector<PlantIndex, BlockIndex> m_plants;
	DataVector<CollisionVolume, BlockIndex> m_dynamicVolume;
	DataVector<CollisionVolume, BlockIndex> m_staticVolume;
	DataVector<FactionIdMap<SmallSet<Project*>>, BlockIndex> m_projects;
	DataVector<SmallMapStable<MaterialTypeId, Fire>*, BlockIndex> m_fires;
	DataVector<TemperatureDelta, BlockIndex> m_temperatureDelta;
	DataVector<LocationBucket*, BlockIndex> m_locationBucket;
	DataVector<std::array<BlockIndex, 6>, BlockIndex> m_directlyAdjacent;
	DataBitSet<BlockIndex> m_exposedToSky;
	DataBitSet<BlockIndex> m_underground;
	DataBitSet<BlockIndex> m_isEdge;
	DataBitSet<BlockIndex> m_outdoors;
	DataBitSet<BlockIndex> m_visible;
	DataBitSet<BlockIndex> m_constructed;
	Area& m_area;
public:
	const DistanceInBlocks m_sizeX;
	const DistanceInBlocks m_sizeY;
	const DistanceInBlocks m_sizeZ;
	Blocks(Area& area, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void resize(BlockIndex count);
	void initalize(BlockIndex index);
	void recordAdjacent(BlockIndex index);
	void assignLocationBuckets();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Cuboid getAll();
	[[nodiscard]] const Cuboid getAll() const;
	[[nodiscard]] size_t size() const;
	[[nodiscard]] BlockIndex getIndex(Point3D coordinates) const;
	BlockIndex maybeGetIndexFromOffset(Point3D coordinates, const std::array<int8_t, 3> offset) const;
	BlockIndex maybeGetIndexFromOffsetOnEdge(Point3D coordinates, const std::array<int8_t, 3> offset) const;
	[[nodiscard]] BlockIndex getIndex(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z) const;
	[[nodiscard]] BlockIndex getIndex_i(uint x, uint y, uint z) const;
	[[nodiscard]] Point3D getCoordinates(BlockIndex index) const;
	Point3D_fractional getCoordinatesFractional(BlockIndex index) const;
	[[nodiscard]] DistanceInBlocks getZ(BlockIndex index) const;
	[[nodiscard]] BlockIndex getAtFacing(BlockIndex index, Facing facing) const;
	[[nodiscard]] BlockIndex getCenterAtGroundLevel() const;
	// TODO: Calculate on demand from offset vector?
	[[nodiscard]] const std::array<BlockIndex, 6>& getDirectlyAdjacent(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<18> getAdjacentWithEdgeAdjacent(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<26> getAdjacentWithEdgeAndCornerAdjacent(BlockIndex index) const;
	[[nodiscard]] std::array<BlockIndex, 26> getAdjacentWithEdgeAndCornerAdjacentUnfiltered(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<20> getEdgeAndCornerAdjacentOnly(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<12> getEdgeAdjacentOnly(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<4> getEdgeAdjacentOnSameZLevelOnly(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<4> getAdjacentOnSameZLevelOnly(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<4> getEdgeAdjacentOnlyOnNextZLevelDown(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<8> getEdgeAndCornerAdjacentOnlyOnNextZLevelDown(BlockIndex index) const;
	[[nodiscard]] BlockIndexArrayNotNull<4> getEdgeAdjacentOnlyOnNextZLevelUp(BlockIndex index) const;
	//TODO: Under what circumstances is this integer distance preferable to taxiDistance or fractional distance?
	[[nodiscard]] DistanceInBlocks distance(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] DistanceInBlocks taxiDistance(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] DistanceInBlocks distanceSquared(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] DistanceInBlocksFractional distanceFractional(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] bool squareOfDistanceIsMoreThen(BlockIndex index, BlockIndex other, DistanceInBlocksFractional distanceSquared) const;
	[[nodiscard]] bool isAdjacentToAny(BlockIndex index, BlockIndices& blocks) const;
	[[nodiscard]] bool isAdjacentTo(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] bool isAdjacentToIncludingCornersAndEdges(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] bool isAdjacentToActor(BlockIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItem(BlockIndex index, ItemIndex item) const;
	[[nodiscard]] bool isConstructed(BlockIndex index) const;
	[[nodiscard]] bool canSeeIntoFromAlways(BlockIndex index, BlockIndex other) const;
	void moveContentsTo(BlockIndex index, BlockIndex other);
	[[nodiscard]] Mass getMass(BlockIndex index) const;
	// Get block at offset coordinates. Can return nullptr.
	[[nodiscard]] BlockIndex offset(BlockIndex index, int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] BlockIndex offsetNotNull(BlockIndex index, int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] BlockIndex indexAdjacentToAtCount(BlockIndex index, AdjacentIndex adjacentCount) const;
	[[nodiscard]] std::array<int32_t, 3> relativeOffsetTo(BlockIndex index, BlockIndex other) const; 
	[[nodiscard]] std::array<int, 26> makeOffsetsForAdjacentCountTable() const;
	[[nodiscard]] bool canSeeThrough(BlockIndex index) const;
	[[nodiscard]] bool canSeeThroughFloor(BlockIndex index) const;
	[[nodiscard]] bool canSeeThroughFrom(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFrom(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFromIncludingDiagonal(BlockIndex index, BlockIndex other, Facing inital = Facing::create(0)) const;
	[[nodiscard]] bool isSupport(BlockIndex index) const;
	[[nodiscard]] bool isOutdoors(BlockIndex index) const;
	[[nodiscard]] bool isExposedToSky(BlockIndex index) const;
	[[nodiscard]] bool isUnderground(BlockIndex index) const;
	[[nodiscard]] bool isEdge(BlockIndex index) const;
	[[nodiscard]] bool isOnSurface(BlockIndex index) const { return isOutdoors(index) && !isUnderground(index); }
	[[nodiscard]] bool hasLineOfSightTo(BlockIndex index, BlockIndex other) const;
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEver(BlockIndex index, ShapeId shape, MoveTypeId moveType) const;
	[[nodiscard]] BlockIndex getBlockBelow(BlockIndex index) const;
	[[nodiscard]] BlockIndex getBlockAbove(BlockIndex index) const;
	[[nodiscard]] BlockIndex getBlockNorth(BlockIndex index) const;
	[[nodiscard]] BlockIndex getBlockWest(BlockIndex index) const;
	[[nodiscard]] BlockIndex getBlockSouth(BlockIndex index) const;
	[[nodiscard]] BlockIndex getBlockEast(BlockIndex index) const;
	[[nodiscard]] LocationBucket& getLocationBucket(BlockIndex index);
	[[nodiscard]] Cuboid getZLevel(DistanceInBlocks z);
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(BlockIndex index, bool exposed);
	void setBelowExposedToSky(BlockIndex index);
	void setBelowNotExposedToSky(BlockIndex index);
	void setBelowVisible(BlockIndex index);
	//TODO: Use std::function instead of template.
	template <typename F>
	[[nodiscard]] BlockIndices collectAdjacentsWithCondition(BlockIndex index, F&& condition)
	{
		BlockIndices output;
		std::stack<BlockIndex> openList;
		openList.push(index);
		output.add(index);
		while(!openList.empty())
		{
			BlockIndex block = openList.top();
			openList.pop();
			for(BlockIndex adjacent : getDirectlyAdjacent(block))
				if(adjacent.exists() && condition(adjacent) && !output.contains(adjacent))
				{
					output.add(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	template <typename F>
	[[nodiscard]] BlockIndex getBlockInRangeWithCondition(BlockIndex index, DistanceInBlocks range, F&& condition)
	{
		std::stack<BlockIndex> open;
		open.push(index);
		BlockIndices closed;
		while(!open.empty())
		{
			BlockIndex block = open.top();
			assert(block.exists());
			if(condition(block))
				return block;
			open.pop();
			for(BlockIndex adjacent : getDirectlyAdjacent(block))
				if(adjacent.exists() && taxiDistance(index, adjacent) <= range && !closed.contains(adjacent))
				{
					closed.add(adjacent);
					open.push(adjacent);
				}
		}
		return BlockIndex::null();
	}
	[[nodiscard]] BlockIndices collectAdjacentsInRange(BlockIndex index, DistanceInBlocks range);
	static inline constexpr std::array<std::array<int8_t, 3>, 26> offsetsListAllAdjacent{{
		{-1,1,-1}, {-1,0,-1}, {-1,-1,-1},
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,1,-1}, {1,0,-1}, {1,-1,-1},

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {-1,1,0},

		{-1,1,1}, {-1,0,1}, {-1,-1,1},
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,1,1}, {1,0,1}, {1,-1,1}
	}};
	// -Designation
	[[nodiscard]] bool designation_has(BlockIndex index, FactionId faction, BlockDesignation designation) const;
	void designation_set(BlockIndex index, FactionId faction, BlockDesignation designation);
	void designation_unset(BlockIndex index, FactionId faction, BlockDesignation designation);
	void designation_maybeUnset(BlockIndex index, FactionId faction, BlockDesignation designation);
	// -Solid.
	void solid_set(BlockIndex index, MaterialTypeId materialType, bool constructed);
	void solid_setNot(BlockIndex index);
	[[nodiscard]] bool solid_is(BlockIndex index) const;
	[[nodiscard]] MaterialTypeId solid_get(BlockIndex index) const;
	// -BlockFeature.
	void blockFeature_construct(BlockIndex index, const BlockFeatureType& featureType, MaterialTypeId materialType);
	void blockFeature_hew(BlockIndex index, const BlockFeatureType& featureType);
	void blockFeature_remove(BlockIndex index, const BlockFeatureType& type);
	void blockFeature_removeAll(BlockIndex index);
	void blockFeature_lock(BlockIndex index, const BlockFeatureType& type);
	void blockFeature_unlock(BlockIndex index, const BlockFeatureType& type);
	void blockFeature_close(BlockIndex index, const BlockFeatureType& type);
	void blockFeature_open(BlockIndex index, const BlockFeatureType& type);
	void blockFeature_setTemperature(BlockIndex index, Temperature temperature);
	[[nodiscard]] const BlockFeature* blockFeature_atConst(BlockIndex index, const BlockFeatureType& blockFeatueType) const;
	[[nodiscard]] BlockFeature* blockFeature_at(BlockIndex index, const BlockFeatureType& blockFeatueType);
	[[nodiscard]] const BlockIndices& blockFeature_get(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_empty(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_blocksEntrance(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canStandAbove(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canStandIn(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_isSupport(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canEnterFromBelow(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canEnterFromAbove(BlockIndex index, BlockIndex from) const;
	[[nodiscard]] MaterialTypeId blockFeature_getMaterialType(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_contains(BlockIndex index, const BlockFeatureType& blockFeatureType) const;
	// -Fluids
	void fluid_spawnMist(BlockIndex index, FluidTypeId fluidType, DistanceInBlocks maxMistSpread = DistanceInBlocks::create(0));
	void fluid_clearMist(BlockIndex index);
	DistanceInBlocks fluid_getMistInverseDistanceToSource(BlockIndex) const;
	FluidGroup* fluid_getGroup(BlockIndex index, FluidTypeId fluidType) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void fluid_add(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType);
	// To be used durring read step.
	void fluid_remove(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType);
	// To be used used durring write step.
	void fluid_removeSyncronus(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType);
	// Move less dense fluids to their group's excessVolume until Config::maxBlockVolume is achieved.
	void fluid_resolveOverfull(BlockIndex index);
	void fluid_onBlockSetSolid(BlockIndex index);
	void fluid_onBlockSetNotSolid(BlockIndex index);
	void fluid_mistSetFluidTypeAndInverseDistance(BlockIndex index, FluidTypeId fluidType, DistanceInBlocks inverseDistance);
	// TODO: This could probably be resolved in a better way.
	// Exposing these two methods breaks encapusalition a bit but allows better performance from fluidGroup.
	void fluid_setAllUnstableExcept(BlockIndex index, FluidTypeId fluidType);
	// To be used by DrainQueue / FillQueue.
	void fluid_drainInternal(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType);
	void fluid_fillInternal(BlockIndex index, CollisionVolume volume, FluidGroup& fluidGroup);
	// To be used by fluid group split, so the blocks which will be in soon to be created groups can briefly have fluid without having a fluidGroup.
	// Fluid group will be set again in addBlock within the new group's constructor.
	// This prevents a problem where addBlock attempts to remove a block from a group which it has already been removed from.
	// TODO: Seems messy.
	void fluid_unsetGroupInternal(BlockIndex index, FluidTypeId fluidType);
	// To be called from FluidGroup::splitStep only.
	[[nodiscard]] bool fluid_undisolveInternal(BlockIndex index, FluidGroup& fluidGroup);
private:void fluid_destroyData(BlockIndex index, FluidTypeId fluidType);
	void fluid_setTotalVolume(BlockIndex index, CollisionVolume volume);
public: [[nodiscard]] bool fluid_canEnterCurrently(BlockIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] bool fluid_isAdjacentToGroup(BlockIndex index, const FluidGroup* fluidGroup) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeCanEnter(BlockIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeContains(BlockIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] FluidTypeId fluid_getTypeWithMostVolume(BlockIndex index) const;
	[[nodiscard]] bool fluid_canEnterEver(BlockIndex index) const;
	[[nodiscard]] bool fluid_typeCanEnterCurrently(BlockIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] bool fluid_any(BlockIndex index) const;
	[[nodiscard]] bool fluid_contains(BlockIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] std::vector<FluidData>& fluid_getAll(BlockIndex index);
	[[nodiscard]] std::vector<FluidData>& fluid_getAllSortedByDensityAscending(BlockIndex index);
	[[nodiscard]] CollisionVolume fluid_getTotalVolume(BlockIndex index) const;
	[[nodiscard]] FluidTypeId fluid_getMist(BlockIndex index) const;
	[[nodiscard]] std::vector<FluidData>::iterator fluid_getDataIterator(BlockIndex index, FluidTypeId fluidType);
	[[nodiscard]] const FluidData* fluid_getData(BlockIndex index, FluidTypeId fluidType) const;
	[[nodiscard]] FluidData* fluid_getData(BlockIndex index, FluidTypeId fluidType);
	// -Fire
	void fire_setPointer(BlockIndex index, SmallMapStable<MaterialTypeId, Fire>* pointer);
	void fire_clearPointer(BlockIndex index);
	[[nodiscard]] bool fire_exists(BlockIndex index) const;
	[[nodiscard]] FireStage fire_getStage(BlockIndex index) const;
	[[nodiscard]] Fire& fire_get(BlockIndex index, MaterialTypeId materialType);
	// -Reservations
	void reserve(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback = nullptr);
	void unreserve(BlockIndex index, CanReserve& canReserve);
	void unreserveAll(BlockIndex index);
	void setReservationDishonorCallback(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	[[nodiscard]] bool isReserved(BlockIndex index, const FactionId faction) const;
	// -Actors
	void actor_record(BlockIndex index, ActorIndex actor, CollisionVolume volume);
	void actor_erase(BlockIndex index, ActorIndex actor);
	void actor_setTemperature(BlockIndex index, Temperature temperature);
	void actor_updateIndex(BlockIndex index, ActorIndex oldIndex, ActorIndex newIndex);
	[[nodiscard]] bool actor_canStandIn(BlockIndex index) const;
	[[nodiscard]] bool actor_contains(BlockIndex index, ActorIndex actor) const;
	[[nodiscard]] bool actor_empty(BlockIndex index) const;
	[[nodiscard]] Volume actor_volumeOf(BlockIndex index, ActorIndex actor) const;
	[[nodiscard]] ActorIndicesForBlock& actor_getAll(BlockIndex index);
	[[nodiscard]] const ActorIndicesForBlock& actor_getAllConst(BlockIndex index) const;
	// -Items
	void item_record(BlockIndex index, ItemIndex item, CollisionVolume volume);
	void item_erase(BlockIndex index, ItemIndex item);
	void item_setTemperature(BlockIndex index, Temperature temperature);
	void item_disperseAll(BlockIndex index);
	void item_updateIndex(BlockIndex index, ItemIndex oldIndex, ItemIndex newIndex);
	ItemIndex item_addGeneric(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	//ItemIndex get(BlockIndex index, ItemType& itemType) const;
	[[nodiscard]] Quantity item_getCount(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType) const;
	[[nodiscard]] ItemIndex item_getGeneric(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType) const;
	[[nodiscard]] ItemIndicesForBlock& item_getAll(BlockIndex index);
	const ItemIndicesForBlock& item_getAll(BlockIndex index) const;
	[[nodiscard]] bool item_hasInstalledType(BlockIndex index, ItemTypeId itemType) const;
	[[nodiscard]] bool item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(BlockIndex index, const ActorIndex actor) const;
	[[nodiscard]] bool item_hasContainerContainingFluidTypeCarryableBy(BlockIndex index, const ActorIndex actor, FluidTypeId fluidType) const;
	[[nodiscard]] bool item_empty(BlockIndex index) const;
	[[nodiscard]] bool item_contains(BlockIndex index, ItemIndex item) const;
	// -Plant
	void plant_create(BlockIndex index, PlantSpeciesId plantSpecies, Percent growthPercent = Percent::null());
	void plant_updateGrowingStatus(BlockIndex index);
	void plant_clearPointer(BlockIndex index);
	void plant_setTemperature(BlockIndex index, Temperature temperature);
	void plant_erase(BlockIndex index);
	void plant_set(BlockIndex index, PlantIndex plant);
	PlantIndex plant_get(BlockIndex index);
	PlantIndex plant_get(BlockIndex index) const;
	bool plant_canGrowHereCurrently(BlockIndex index, PlantSpeciesId plantSpecies) const;
	bool plant_canGrowHereAtSomePointToday(BlockIndex index, PlantSpeciesId plantSpecies) const;
	bool plant_canGrowHereEver(BlockIndex index, PlantSpeciesId plantSpecies) const;
	bool plant_anythingCanGrowHereEver(BlockIndex index) const;
	bool plant_exists(BlockIndex index) const;
	// -Shape / Move
	void shape_addStaticVolume(BlockIndex index, CollisionVolume volume);
	void shape_removeStaticVolume(BlockIndex index, CollisionVolume volume);
	[[nodiscard]] bool shape_anythingCanEnterEver(BlockIndex index) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverFrom(BlockIndex index, ShapeId shape, MoveTypeId moveType, const BlockIndex block) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithFacing(BlockIndex index, ShapeId shape, MoveTypeId moveType, const Facing facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(BlockIndex index, ShapeId shape, MoveTypeId moveType) const;
	// CanEnterCurrently methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(BlockIndex index, ShapeId shape, MoveTypeId moveType, const Facing facing, const BlockIndices& occupied) const;
	[[nodiscard]] Facing shape_canEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, ShapeId shape, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(BlockIndex index, ShapeId shape, MoveTypeId moveType, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithAnyFacing(BlockIndex index, ShapeId shape, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyFrom(BlockIndex index, ShapeId shape, BlockIndex other, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithFacing(BlockIndex index, ShapeId shape, Facing facing, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_moveTypeCanEnter(BlockIndex index, MoveTypeId moveType) const;
	[[nodiscard]] bool shape_moveTypeCanEnterFrom(BlockIndex index, MoveTypeId moveType, const BlockIndex from) const;
	[[nodiscard]] bool shape_moveTypeCanBreath(BlockIndex index, MoveTypeId moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithFacing(BlockIndex index, ShapeId Shape, const Facing& facing, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithAnyFacing(BlockIndex index, ShapeId shape, const BlockIndices& occupied) const;
	[[nodiscard]] std::pair<bool, Facing> shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, ShapeId shape, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithFacing(BlockIndex index, ShapeId shape, Facing facing, const BlockIndices& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithAnyFacing(BlockIndex index, ShapeId shape, const BlockIndices& occupied) const;
	[[nodiscard]] MoveCost shape_moveCostFrom(BlockIndex index, MoveTypeId moveType, const BlockIndex from) const;
	[[nodiscard]] bool shape_canStandIn(BlockIndex index) const;
	[[nodiscard]] CollisionVolume shape_getDynamicVolume(BlockIndex index) const;
	[[nodiscard]] CollisionVolume shape_getStaticVolume(BlockIndex index) const;
	[[nodiscard]] Quantity shape_getQuantityOfItemWhichCouldFit(BlockIndex index, ItemTypeId itemType) const;
	// -FarmField
	void farm_insert(BlockIndex index, FactionId faction, FarmField& farmField);
	void farm_remove(BlockIndex index, FactionId faction);
	void farm_designateForHarvestIfPartOfFarmField(BlockIndex index, PlantIndex plant);
	void farm_designateForGiveFluidIfPartOfFarmField(BlockIndex index, PlantIndex plant);
	void farm_maybeDesignateForSowingIfPartOfFarmField(BlockIndex index);
	void farm_removeAllHarvestDesignations(BlockIndex index);
	void farm_removeAllGiveFluidDesignations(BlockIndex index);
	void farm_removeAllSowSeedsDesignations(BlockIndex index);
	[[nodiscard]] bool farm_isSowingSeasonFor(PlantSpeciesId species) const;
	[[nodiscard]] bool farm_contains(BlockIndex index, FactionId faction) const;
	[[nodiscard]] FarmField* farm_get(BlockIndex index, FactionId faction);
	[[nodiscard]] const FarmField* farm_get(BlockIndex index, FactionId faction) const;
	// -StockPile
	void stockpile_recordMembership(BlockIndex index, StockPile& stockPile);
	void stockpile_recordNoLongerMember(BlockIndex index, StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void stockpile_updateActive(BlockIndex index);
	[[nodiscard]] StockPile* stockpile_getForFaction(BlockIndex index, FactionId faction);
	[[nodiscard]] const StockPile* stockpile_getForFaction(BlockIndex index, FactionId faction) const;
	[[nodiscard]] bool stockpile_contains(BlockIndex index, FactionId faction) const;
	[[nodiscard]] bool stockpile_isAvalible(BlockIndex index, FactionId faction) const;
	// -Project
	void project_add(BlockIndex index, Project& project);
	void project_remove(BlockIndex index, Project& project);
	Percent project_getPercentComplete(BlockIndex index, FactionId faction) const;
	[[nodiscard]] Project* project_get(BlockIndex index, FactionId faction) const;
	[[nodiscard]] Project* project_getIfBegun(BlockIndex index, FactionId faction) const;
	// -Temperature
	void temperature_freeze(BlockIndex index, FluidTypeId fluidType);
	void temperature_melt(BlockIndex index);
	void temperature_apply(BlockIndex index, Temperature temperature, TemperatureDelta delta);
	void temperature_updateDelta(BlockIndex index, TemperatureDelta delta);
	const Temperature& temperature_getAmbient(BlockIndex index) const;
	Temperature temperature_getDailyAverageAmbient(BlockIndex index) const;
	Temperature temperature_get(BlockIndex index) const;
	Blocks(Blocks&) = delete;
	Blocks(Blocks&&) = delete;
};
