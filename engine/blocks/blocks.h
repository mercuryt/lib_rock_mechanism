#pragma once

#include "../fire.h"
#include "../fluidType.h"
#include "../reservable.h"
#include "../stockpile.h"
#include "../farmFields.h"
#include "../types.h"
#include "../designations.h"
#include "../blockFeature.h"
#include "../bloomHashMap.h"

#include "../../lib/dynamic_bitset.hpp"

#include <unordered_map>
#include <vector>
#include <memory>

struct ItemType;
struct MaterialType;
struct MoveType;
struct PlantSpecies;
struct FluidType;
struct Faction;
struct Shape;
class FluidGroup;
class LocationBucket;

struct FluidData
{
	const FluidType* type;
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
class Blocks
{
	std::array<int32_t, 26> m_offsetsForAdjacentCountTable;
	BloomHashMap<BlockIndex, Reservable, 100'000> m_reservables;
	std::unordered_map<BlockIndex, std::unordered_map<Faction*, FarmField*>> m_farmFields;
	std::unordered_map<BlockIndex, std::unordered_map<Faction*, BlockIsPartOfStockPile>> m_stockPiles;
	std::vector<const MaterialType*> m_materialType;
	std::vector<std::vector<BlockFeature>> m_features;
	std::vector<std::vector<FluidData>> m_fluid;
	std::vector<const FluidType*> m_mist;
	std::vector<CollisionVolume> m_totalFluidVolume;
	std::vector<DistanceInBlocks> m_mistInverseDistanceFromSource;
	std::vector<std::vector<std::pair<ActorIndex, CollisionVolume>>> m_actors;
	std::vector<std::vector<std::pair<ItemIndex, CollisionVolume>>> m_items;
	std::vector<PlantIndex> m_plants;
	std::vector<CollisionVolume> m_dynamicVolume;
	std::vector<CollisionVolume> m_staticVolume;
	std::vector<std::unordered_map<Faction*, std::unordered_set<Project*>>> m_projects;
	std::vector<std::unordered_map<const MaterialType* , Fire>*> m_fires;
	std::vector<Temperature> m_temperatureDelta;
	std::vector<LocationBucket*> m_locationBucket;
	std::vector<std::array<BlockIndex, 6>> m_directlyAdjacent;
	sul::dynamic_bitset<> m_exposedToSky;
	sul::dynamic_bitset<> m_underground;
	sul::dynamic_bitset<> m_isEdge;
	sul::dynamic_bitset<> m_outdoors;
	sul::dynamic_bitset<> m_visible;
	sul::dynamic_bitset<> m_constructed;
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
	[[nodiscard]] BlockIndex getIndex(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z) const;
	[[nodiscard]] Point3D getCoordinates(BlockIndex index) const;
	[[nodiscard]] DistanceInBlocks getZ(BlockIndex index) const;
	[[nodiscard]] BlockIndex getAtFacing(BlockIndex index, Facing facing) const;
	[[nodiscard]] BlockIndex getCenterAtGroundLevel() const;
	// TODO: change to slice, use single long vector.
	[[nodiscard]] const std::array<BlockIndex, 6>& getDirectlyAdjacent(BlockIndex index) const;
	//TODO: change to return array?
	[[nodiscard]] std::vector<BlockIndex> getAdjacentWithEdgeAdjacent(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getAdjacentWithEdgeAndCornerAdjacent(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getAdjacentWithEdgeAndCornerAdjacentUnfiltered(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getEdgeAndCornerAdjacentOnly(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getEdgeAdjacentOnly(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getEdgeAdjacentOnSameZLevelOnly(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getAdjacentOnSameZLevelOnly(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getEdgeAdjacentOnlyOnNextZLevelDown(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getEdgeAndCornerAdjacentOnlyOnNextZLevelDown(BlockIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getEdgeAdjacentOnlyOnNextZLevelUp(BlockIndex index) const;
	[[nodiscard]] DistanceInBlocks distance(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] DistanceInBlocks taxiDistance(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] bool squareOfDistanceIsMoreThen(BlockIndex index, BlockIndex other, DistanceInBlocks distanceSquared) const;
	[[nodiscard]] bool isAdjacentToAny(BlockIndex index, std::unordered_set<BlockIndex>& blocks) const;
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
	[[nodiscard]] BlockIndex indexAdjacentToAtCount(BlockIndex index, uint8_t adjacentCount) const;
	[[nodiscard]] std::array<int32_t, 3> relativeOffsetTo(BlockIndex index, BlockIndex other) const; 
	[[nodiscard]] std::array<int, 26> makeOffsetsForAdjacentCountTable() const;
	[[nodiscard]] bool canSeeThrough(BlockIndex index) const;
	[[nodiscard]] bool canSeeThroughFloor(BlockIndex index) const;
	[[nodiscard]] bool canSeeThroughFrom(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFrom(BlockIndex index, BlockIndex other) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFromIncludingDiagonal(BlockIndex index, BlockIndex other, Facing inital = 0) const;
	[[nodiscard]] bool isSupport(BlockIndex index) const;
	[[nodiscard]] bool isOutdoors(BlockIndex index) const;
	[[nodiscard]] bool isExposedToSky(BlockIndex index) const;
	[[nodiscard]] bool isUnderground(BlockIndex index) const;
	[[nodiscard]] bool isEdge(BlockIndex index) const;
	[[nodiscard]] bool isOnSurface(BlockIndex index) const { return isOutdoors(index) && !isUnderground(index); }
	[[nodiscard]] bool hasLineOfSightTo(BlockIndex index, BlockIndex other) const;
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEver(BlockIndex index, const Shape& shape, const MoveType& moveType) const;
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
	[[nodiscard]] std::unordered_set<BlockIndex> collectAdjacentsWithCondition(BlockIndex index, F&& condition)
	{
		std::unordered_set<BlockIndex> output;
		std::stack<BlockIndex> openList;
		openList.push(index);
		output.insert(index);
		while(!openList.empty())
		{
			BlockIndex block = openList.top();
			openList.pop();
			for(BlockIndex adjacent : getDirectlyAdjacent(block))
				if(adjacent != BLOCK_INDEX_MAX && condition(adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
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
		std::unordered_set<BlockIndex> closed;
		while(!open.empty())
		{
			BlockIndex block = open.top();
			if(condition(block != BLOCK_INDEX_MAX))
				return block;
			open.pop();
			for(BlockIndex adjacent : getDirectlyAdjacent(block))
				if(adjacent != BLOCK_INDEX_MAX && taxiDistance(index, adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
					
				
		}
		return BLOCK_INDEX_MAX;
	}
	[[nodiscard]] std::unordered_set<BlockIndex> collectAdjacentsInRange(BlockIndex index, DistanceInBlocks range);
	[[nodiscard]] std::vector<BlockIndex> collectAdjacentsInRangeVector(BlockIndex index, DistanceInBlocks range);
	static inline const int32_t offsetsListAllAdjacent[26][3] = {
		{-1,1,-1}, {-1,0,-1}, {-1,-1,-1},
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,1,-1}, {1,0,-1}, {1,-1,-1},

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {-1,1,0},

		{-1,1,1}, {-1,0,1}, {-1,-1,1},
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,1,1}, {1,0,1}, {1,-1,1}
	};
	// -Designation
	[[nodiscard]] bool designation_has(BlockIndex index, Faction& faction, BlockDesignation designation) const;
	void designation_set(BlockIndex index, Faction& faction, BlockDesignation designation);
	void designation_unset(BlockIndex index, Faction& faction, BlockDesignation designation);
	void designation_maybeUnset(BlockIndex index, Faction& faction, BlockDesignation designation);
	// -Solid.
	void solid_set(BlockIndex index, const MaterialType& materialType, bool constructed);
	void solid_setNot(BlockIndex index);
	[[nodiscard]] bool solid_is(BlockIndex index) const;
	[[nodiscard]] const MaterialType& solid_get(BlockIndex index) const;
	// -BlockFeature.
	void blockFeature_construct(BlockIndex index, const BlockFeatureType& featureType, const MaterialType& materialType);
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
	[[nodiscard]] const std::vector<BlockFeature>& blockFeature_get(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_empty(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_blocksEntrance(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canStandAbove(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canStandIn(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_isSupport(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canEnterFromBelow(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_canEnterFromAbove(BlockIndex index, BlockIndex from) const;
	[[nodiscard]] const MaterialType* blockFeature_getMaterialType(BlockIndex index) const;
	[[nodiscard]] bool blockFeature_contains(BlockIndex index, const BlockFeatureType& blockFeatureType) const;
	// -Fluids
	void fluid_spawnMist(BlockIndex index, const FluidType& fluidType, DistanceInBlocks maxMistSpread = 0);
	void fluid_clearMist(BlockIndex index);
	DistanceInBlocks fluid_getMistInverseDistanceToSource(BlockIndex) const;
	FluidGroup* fluid_getGroup(BlockIndex index, const FluidType& fluidType) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void fluid_add(BlockIndex index, CollisionVolume volume, const FluidType& fluidType);
	// To be used durring read step.
	void fluid_remove(BlockIndex index, CollisionVolume volume, const FluidType& fluidType);
	// To be used used durring write step.
	void fluid_removeSyncronus(BlockIndex index, CollisionVolume volume, const FluidType& fluidType);
	// Move less dense fluids to their group's excessVolume until Config::maxBlockVolume is achieved.
	void fluid_resolveOverfull(BlockIndex index);
	void fluid_onBlockSetSolid(BlockIndex index);
	void fluid_onBlockSetNotSolid(BlockIndex index);
	void fluid_mistSetFluidTypeAndInverseDistance(BlockIndex index, const FluidType& fluidType, DistanceInBlocks inverseDistance);
	// TODO: This could probably be resolved in a better way.
	// Exposing these two methods breaks encapusalition a bit but allows better performance from fluidGroup.
	void fluid_setAllUnstableExcept(BlockIndex index, const FluidType& fluidType);
	// To be used by DrainQueue / FillQueue.
	void fluid_drainInternal(BlockIndex index, CollisionVolume volume, const FluidType& fluidType);
	void fluid_fillInternal(BlockIndex index, CollisionVolume volume, FluidGroup& fluidGroup);
	[[nodiscard]] bool fluid_undisolveInternal(BlockIndex index, FluidGroup& fluidGroup);
private:void fluid_destroyData(BlockIndex index, const FluidType& fluidType);
public: [[nodiscard]] bool fluid_canEnterCurrently(BlockIndex index, const FluidType& fluidType) const;
	[[nodiscard]] bool fluid_isAdjacentToGroup(BlockIndex index, const FluidGroup* fluidGroup) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeCanEnter(BlockIndex index, const FluidType& fluidType) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeContains(BlockIndex index, const FluidType& fluidType) const;
	[[nodiscard]] const FluidType& fluid_getTypeWithMostVolume(BlockIndex index) const;
	[[nodiscard]] bool fluid_canEnterEver(BlockIndex index) const;
	[[nodiscard]] bool fluid_typeCanEnterCurrently(BlockIndex index, const FluidType& fluidType) const;
	[[nodiscard]] bool fluid_any(BlockIndex index) const;
	[[nodiscard]] bool fluid_contains(BlockIndex index, const FluidType& fluidType) const;
	[[nodiscard]] std::vector<FluidData>& fluid_getAll(BlockIndex index);
	[[nodiscard]] std::vector<FluidData>& fluid_getAllSortedByDensityAscending(BlockIndex index);
	[[nodiscard]] CollisionVolume fluid_getTotalVolume(BlockIndex index) const;
	[[nodiscard]] const FluidType* fluid_getMist(BlockIndex index) const;
	[[nodiscard]] std::vector<FluidData>::iterator fluid_getDataIterator(BlockIndex index, const FluidType& fluidType);
	[[nodiscard]] const FluidData* fluid_getData(BlockIndex index, const FluidType& fluidType) const;
	[[nodiscard]] FluidData* fluid_getData(BlockIndex index, const FluidType& fluidType);
	// -Fire
	void fire_setPointer(BlockIndex index, std::unordered_map<const MaterialType* , Fire>* pointer);
	void fire_clearPointer(BlockIndex index);
	[[nodiscard]] bool fire_exists(BlockIndex index) const;
	[[nodiscard]] FireStage fire_getStage(BlockIndex index) const;
	[[nodiscard]] Fire& fire_get(BlockIndex index, const MaterialType& materialType);
	// -Reservations
	void reserve(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback = nullptr);
	void unreserve(BlockIndex index, CanReserve& canReserve);
	void unreserveAll(BlockIndex index);
	void setReservationDishonorCallback(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	[[nodiscard]] bool isReserved(BlockIndex index, const Faction& faction) const;
	// -Actors
	void actor_record(BlockIndex index, ActorIndex actor, CollisionVolume volume);
	void actor_erase(BlockIndex index, ActorIndex actor);
	void actor_setTemperature(BlockIndex index, Temperature temperature);
	[[nodiscard]] bool actor_canStandIn(BlockIndex index) const;
	[[nodiscard]] bool actor_contains(BlockIndex index, ActorIndex actor) const;
	[[nodiscard]] bool actor_empty(BlockIndex index) const;
	[[nodiscard]] Volume actor_volumeOf(BlockIndex index, ActorIndex actor) const;
	[[nodiscard]] std::vector<ActorIndex>& actor_getAll(BlockIndex index);
	[[nodiscard]] const std::vector<ActorIndex>& actor_getAllConst(BlockIndex index) const;
	// -Items
	void item_record(BlockIndex index, ItemIndex item, CollisionVolume volume);
	void item_erase(BlockIndex index, ItemIndex item);
	void item_setTemperature(BlockIndex index, Temperature temperature);
	void item_disperseAll(BlockIndex index);
	ItemIndex item_addGeneric(BlockIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const;
	//ItemIndex get(BlockIndex index, ItemType& itemType) const;
	[[nodiscard]] Quantity item_getCount(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const;
	[[nodiscard]] ItemIndex item_getGeneric(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const;
	[[nodiscard]] std::vector<ItemIndex> item_getAll(BlockIndex index);
	const std::vector<ItemIndex> item_getAll(BlockIndex index) const;
	[[nodiscard]] bool item_hasInstalledType(BlockIndex index, const ItemType& itemType) const;
	[[nodiscard]] bool item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(BlockIndex index, const ActorIndex actor) const;
	[[nodiscard]] bool item_hasContainerContainingFluidTypeCarryableBy(BlockIndex index, const ActorIndex actor, const FluidType& fluidType) const;
	[[nodiscard]] bool item_empty(BlockIndex index) const;
	// -Plant
	void plant_create(BlockIndex index, const PlantSpecies& plantSpecies, Percent growthPercent = 0);
	void plant_updateGrowingStatus(BlockIndex index);
	void plant_clearPointer(BlockIndex index);
	void plant_setTemperature(BlockIndex index, Temperature temperature);
	void plant_erase(BlockIndex index);
	void plant_set(BlockIndex index, PlantIndex plant);
	PlantIndex plant_get(BlockIndex index);
	PlantIndex plant_get(BlockIndex index) const;
	bool plant_canGrowHereCurrently(BlockIndex index, const PlantSpecies& plantSpecies) const;
	bool plant_canGrowHereAtSomePointToday(BlockIndex index, const PlantSpecies& plantSpecies) const;
	bool plant_canGrowHereEver(BlockIndex index, const PlantSpecies& plantSpecies) const;
	bool plant_anythingCanGrowHereEver(BlockIndex index) const;
	bool plant_exists(BlockIndex index) const;
	// -Shape / Move
	void shape_addStaticVolume(BlockIndex index, CollisionVolume volume);
	void shape_removeStaticVolume(BlockIndex index, CollisionVolume volume);
	[[nodiscard]] bool shape_anythingCanEnterEver(BlockIndex index) const;
	// CanEnter methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(BlockIndex index, const Shape& shape, const MoveType& moveType, const Facing facing, std::vector<BlockIndex>& occupied) const;
	[[nodiscard]] std::pair<bool, Facing> shape_canEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, const Shape& shape, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithAnyFacing(BlockIndex index, const Shape& shape, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverFrom(BlockIndex index, const Shape& shape, const MoveType& moveType, const BlockIndex block) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithFacing(BlockIndex index, const Shape& shape, const MoveType& moveType, const Facing facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(BlockIndex index, const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] bool shape_canEnterCurrentlyFrom(BlockIndex index, const Shape& shape, BlockIndex other, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithFacing(BlockIndex index, const Shape& shape, Facing facing, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_moveTypeCanEnter(BlockIndex index, const MoveType& moveType) const;
	[[nodiscard]] bool shape_moveTypeCanEnterFrom(BlockIndex index, const MoveType& moveType, const BlockIndex from) const;
	[[nodiscard]] bool shape_moveTypeCanBreath(BlockIndex index, const MoveType& moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithFacing(BlockIndex index, const Shape& Shape, const Facing& facing, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithAnyFacing(BlockIndex index, const Shape& shape, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] std::pair<bool, Facing> shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, const Shape& shape, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithFacing(BlockIndex index, const Shape& shape, Facing facing, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithAnyFacing(BlockIndex index, const Shape& shape, std::unordered_set<BlockIndex>& occupied) const;
	[[nodiscard]] MoveCost shape_moveCostFrom(BlockIndex index, const MoveType& moveType, const BlockIndex from) const;
	[[nodiscard]] bool shape_canStandIn(BlockIndex index) const;
	[[nodiscard]] CollisionVolume shape_getDynamicVolume(BlockIndex index) const;
	[[nodiscard]] bool shape_contains(BlockIndex index, Shape& shape) const;
	[[nodiscard]] CollisionVolume shape_getStaticVolume(BlockIndex index) const;
	[[nodiscard]] std::unordered_map<Shape*, CollisionVolume>& shape_getShapes(BlockIndex index);
	[[nodiscard]] Quantity shape_getQuantityOfItemWhichCouldFit(BlockIndex index, const ItemType& itemType) const;
	// -FarmField
	void farm_insert(BlockIndex index, Faction& faction, FarmField& farmField);
	void farm_remove(BlockIndex index, Faction& faction);
	void farm_designateForHarvestIfPartOfFarmField(BlockIndex index, PlantIndex plant);
	void farm_designateForGiveFluidIfPartOfFarmField(BlockIndex index, PlantIndex plant);
	void farm_maybeDesignateForSowingIfPartOfFarmField(BlockIndex index);
	void farm_removeAllHarvestDesignations(BlockIndex index);
	void farm_removeAllGiveFluidDesignations(BlockIndex index);
	void farm_removeAllSowSeedsDesignations(BlockIndex index);
	[[nodiscard]] bool farm_isSowingSeasonFor(const PlantSpecies& species) const;
	[[nodiscard]] bool farm_contains(BlockIndex index, Faction& faction) const;
	[[nodiscard]] FarmField* farm_get(BlockIndex index, Faction& faction);
	// -StockPile
	void stockpile_recordMembership(BlockIndex index, StockPile& stockPile);
	void stockpile_recordNoLongerMember(BlockIndex index, StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void stockpile_updateActive(BlockIndex index);
	[[nodiscard]] StockPile* stockpile_getForFaction(BlockIndex index, Faction& faction);
	[[nodiscard]] bool stockpile_contains(BlockIndex index, Faction& faction) const;
	[[nodiscard]] bool stockpile_isAvalible(BlockIndex index, Faction& faction) const;
	// -Project
	void project_add(BlockIndex index, Project& project);
	void project_remove(BlockIndex index, Project& project);
	Percent project_getPercentComplete(BlockIndex index, Faction& faction) const;
	[[nodiscard]] Project* project_get(BlockIndex index, Faction& faction) const;
	// -Temperature
	void temperature_freeze(BlockIndex index, const FluidType& fluidType);
	void temperature_melt(BlockIndex index);
	void temperature_apply(BlockIndex index, Temperature temperature, const int32_t& delta);
	void temperature_updateDelta(BlockIndex index, int32_t delta);
	const Temperature& temperature_getAmbient(BlockIndex index) const;
	Temperature temperature_getDailyAverageAmbient(BlockIndex index) const;
	Temperature temperature_get(BlockIndex index) const;
};
