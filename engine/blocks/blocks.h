#pragma once

#include "../fire.h"
#include "../fluidType.h"
#include "../reservable.h"
#include "../area/stockpile.h"
#include "../farmFields.h"
#include "../numericTypes/types.h"
#include "../hasShapeTypes.h"
#include "../designations.h"
#include "../blockFeature.h"
#include "../numericTypes/index.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/strongVector.h"
#include "../numericTypes/index.h"
#include "../geometry/offsetSetSIMD.h"
#include "../geometry/pointSet.h"
#include "blockIndexSetSIMD.h"

#include "blockIndexArray.h"
#include "exposedToSky.h"

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
class Blocks
{
	// Shape data.
	OffsetArraySIMD<26> m_indexOffsetsForAdjacentAll;
	OffsetArraySIMD<24> m_indexOffsetsForAdjacentAllExceptDirectlyAboveAndBelow;
	OffsetArraySIMD<20> m_indexOffsetsForAdjacentOnlyEdgesAndCorners;
	OffsetArraySIMD<18> m_indexOffsetsForAdjacentDirectAndEdge;
	OffsetArraySIMD<12> m_indexOffsetsForAdjacentEdge;
	OffsetArraySIMD<6> m_indexOffsetsForAdjacentDirect;
	OffsetArraySIMD<4> m_indexOffsetsForAdjacentEdgeSameZ;
	OffsetArraySIMD<4> m_indexOffsetsForAdjacentDirectSameZ;
	std::vector<OffsetSetSIMD> m_indexOffsetsForNthAdjacent;
	// Block data.
	SmallMap<BlockIndex, SmallMap<FactionId, FarmField*>> m_farmFields;
	SmallMap<BlockIndex, SmallMap<FactionId, BlockIsPartOfStockPile>> m_stockPiles;
	StrongVector<std::unique_ptr<Reservable>, BlockIndex> m_reservables;
	StrongVector<MaterialTypeId, BlockIndex> m_materialType;
	StrongVector<BlockFeatureSet, BlockIndex> m_features;
	StrongVector<std::vector<FluidData>, BlockIndex> m_fluid;
	StrongVector<FluidTypeId, BlockIndex> m_mist;
	StrongVector<CollisionVolume, BlockIndex> m_totalFluidVolume;
	StrongVector<DistanceInBlocks, BlockIndex> m_mistInverseDistanceFromSource;
	//TODO: make these SmallMaps.
	StrongVector<std::vector<std::pair<ActorIndex, CollisionVolume>>, BlockIndex> m_actorVolume;
	StrongVector<std::vector<std::pair<ItemIndex, CollisionVolume>>, BlockIndex> m_itemVolume;
	StrongVector<SmallSet<ActorIndex>, BlockIndex> m_actors;
	StrongVector<SmallSet<ItemIndex>, BlockIndex> m_items;
	StrongBitSet<BlockIndex> m_hasActors;
	StrongBitSet<BlockIndex> m_hasItems;
	StrongVector<PlantIndex, BlockIndex> m_plants;
	StrongVector<CollisionVolume, BlockIndex> m_dynamicVolume;
	StrongVector<CollisionVolume, BlockIndex> m_staticVolume;
	StrongVector<SmallMap<FactionId, SmallSet<Project*>>, BlockIndex> m_projects;
	StrongVector<SmallMapStable<MaterialTypeId, Fire>*, BlockIndex> m_fires;
	StrongVector<TemperatureDelta, BlockIndex> m_temperatureDelta;
	BlocksExposedToSky m_exposedToSky;
	StrongBitSet<BlockIndex> m_isEdge;
	StrongBitSet<BlockIndex> m_visible;
	StrongBitSet<BlockIndex> m_constructed;
	StrongBitSet<BlockIndex> m_dynamic;
	Area& m_area;
public:
	const Coordinates m_pointToIndexConversionMultipliers;
	const Coordinates m_pointToIndexConversionMultipliersChunked;
	const Coordinates m_dimensions;
	uint m_sizeXInChunks;
	uint m_sizeXTimesYInChunks;
	//TODO: replace these with functions accessing m_dimensions.
	const DistanceInBlocks m_sizeX;
	const DistanceInBlocks m_sizeY;
	const DistanceInBlocks m_sizeZ;
	const DistanceInBlocksWidth m_zLevelSize;
	Blocks(Area& area, const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void resize(const BlockIndex& count);
	void initalize(const BlockIndex& index);
	void makeIndexOffsetsForAdjacent();
	void moveContentsTo(const BlockIndex& index, const BlockIndex& other);
	void maybeContentsFalls(const BlockIndex& index);
	void setDynamic(const BlockIndex& index) { m_dynamic.set(index); }
	void unsetDynamic(const BlockIndex& index) { m_dynamic.unset(index); }
	// For testing.
	[[nodiscard]] std::vector<BlockIndex> getAllIndices() const ;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Cuboid getAll();
	[[nodiscard]] const Cuboid getAll() const;
	[[nodiscard]] size_t size() const;
	[[nodiscard]] size_t getChunkedSize() const;
	[[nodiscard]] BlockIndex getIndex(const Point3D& coordinates) const;
	[[nodiscard]] BlockIndex maybeGetIndex(const Point3D& coordinates) const;
	[[nodiscard]] BlockIndex maybeGetIndexFromOffset(const Point3D& coordinates, const Offset3D& offset) const;
	[[nodiscard]] BlockIndex maybeGetIndexFromOffsetOnEdge(const Point3D& coordinates, const Offset3D& offset) const;
	[[nodiscard]] BlockIndex getIndex(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) const;
	// For testing and debug.
	[[nodiscard]] __attribute__((noinline)) BlockIndex getIndex_i(const uint& x, const uint& y, const uint& z) const;
	[[nodiscard]] Point3D getCoordinates(const BlockIndex& index) const;
	[[nodiscard]] Point3DSet getCoordinateSet(const std::vector<BlockIndex>& index) const;
	[[nodiscard]] Point3D_fractional getCoordinatesFractional(const BlockIndex& index) const;
	[[nodiscard]] DistanceInBlocks getZ(const BlockIndex& index) const;
	[[nodiscard]] BlockIndex getAtFacing(const BlockIndex& index, const Facing6& facing) const;
	[[nodiscard]] BlockIndex getCenterAtGroundLevel() const;
	// TODO: Calculate on demand from offset vector?
	[[nodiscard]] BlockIndexArraySIMD<26> getAdjacentWithEdgeAndCornerAdjacent(const BlockIndex& index) const;
	[[nodiscard]] BlockIndexArraySIMD<24> getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(const BlockIndex& index) const;
	[[nodiscard]] BlockIndexArraySIMD<20> getEdgeAndCornerAdjacentOnly(const BlockIndex& index) const;
	[[nodiscard]] BlockIndexArraySIMD<18> getAdjacentWithEdgeAdjacent(const BlockIndex& index) const;
	[[nodiscard]] BlockIndexArraySIMD<12> getEdgeAdjacentOnly(const BlockIndex& index) const;
	//TODO Why does making this a simd set break cave in?
	[[nodiscard]] BlockIndexArraySIMD<6> getDirectlyAdjacent(const BlockIndex& index) const;
	[[nodiscard]] BlockIndexArraySIMD<4> getEdgeAdjacentOnSameZLevelOnly(const BlockIndex& index) const;
	[[nodiscard]] BlockIndexArraySIMD<4> getAdjacentOnSameZLevelOnly(const BlockIndex& index) const;
	// getNthAdjacent is not const because the index offsets are created and cached.
	[[nodiscard]] BlockIndexSetSIMD getNthAdjacent(const BlockIndex& index, const DistanceInBlocks& distance);
	//TODO: Under what circumstances is this integer distance preferable to taxiDistance or fractional distance?
	[[nodiscard]] DistanceInBlocks distance(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] DistanceInBlocks taxiDistance(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] DistanceInBlocks distanceSquared(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] DistanceInBlocksFractional distanceFractional(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] bool squareOfDistanceIsMoreThen(const BlockIndex& index, const BlockIndex& other, DistanceInBlocksFractional distanceSquared) const;
	[[nodiscard]] bool isAdjacentToAny(const BlockIndex& index, SmallSet<BlockIndex>& blocks) const;
	[[nodiscard]] bool isAdjacentTo(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] bool isAdjacentToIncludingCornersAndEdges(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] bool isAdjacentToActor(const BlockIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItem(const BlockIndex& index, const ItemIndex& item) const;
	[[nodiscard]] bool isConstructed(const BlockIndex& index) const { return m_constructed[index]; }
	[[nodiscard]] bool isDynamic(const BlockIndex& index) const { return m_dynamic[index]; }
	[[nodiscard]] bool canSeeIntoFromAlways(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] bool isVisible(const BlockIndex& index) const { return m_visible[index]; }
	// Get block at offset coordinates. Can return nullptr.
	[[nodiscard]] BlockIndex offset(const BlockIndex& index, int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] BlockIndex offset(const BlockIndex& index, const Offset3D& offset) const;
	[[nodiscard]] BlockIndex offsetNotNull(const BlockIndex& index, int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] BlockIndex offsetRotated(const BlockIndex& index, const Offset3D& offset, const Facing4& previousFacing, const Facing4& newFacing) const;
	[[nodiscard]] BlockIndex offsetRotated(const BlockIndex& index, const Offset3D& offset, const Facing4& facing) const;
	[[nodiscard]] BlockIndex indexAdjacentToAtCount(const BlockIndex& index, const AdjacentIndex& adjacentCount) const;
	[[nodiscard]] Offset3D relativeOffsetTo(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] BlockIndex translatePosition(const BlockIndex& location, const BlockIndex& previousPivot, const BlockIndex& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const;
	[[nodiscard]] bool canSeeThrough(const BlockIndex& index) const;
	[[nodiscard]] bool canSeeThroughFloor(const BlockIndex& index) const;
	[[nodiscard]] bool canSeeThroughFrom(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] Facing4 facingToSetWhenEnteringFrom(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] Facing8 facingToSetWhenEnteringFromIncludingDiagonal(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] bool isSupport(const BlockIndex& index) const;
	[[nodiscard]] bool isExposedToSky(const BlockIndex& index) const;
	[[nodiscard]] bool isEdge(const BlockIndex& index) const;
	//TODO: remove this once we replace BlockIndex with Point3D.
	[[nodiscard]] bool hasLineOfSightTo(const BlockIndex& index, const BlockIndex& other) const;
	[[nodiscard]] BlockIndex getBlockBelow(const BlockIndex& index) const;
	[[nodiscard]] BlockIndex getBlockAbove(const BlockIndex& index) const;
	[[nodiscard]] BlockIndex getBlockNorth(const BlockIndex& index) const;
	[[nodiscard]] BlockIndex getBlockWest(const BlockIndex& index) const;
	[[nodiscard]] BlockIndex getBlockSouth(const BlockIndex& index) const;
	[[nodiscard]] BlockIndex getBlockEast(const BlockIndex& index) const;
	//TODO: make this const after removing the area pointer from cuboid.
	[[nodiscard]] Cuboid getZLevel(const DistanceInBlocks& z);
	[[nodiscard]] BlockIndex getMiddleAtGroundLevel() const;
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(const BlockIndex& index, bool exposed);
	void setBelowVisible(const BlockIndex& index);
	//TODO: Use std::function instead of template.
	template <typename F>
	[[nodiscard]] SmallSet<BlockIndex> collectAdjacentsWithCondition(const BlockIndex& index, F&& condition)
	{
		SmallSet<BlockIndex> output;
		std::stack<BlockIndex> openList;
		openList.push(index);
		output.insert(index);
		while(!openList.empty())
		{
			BlockIndex block = openList.top();
			openList.pop();
			for(const BlockIndex& adjacent : getDirectlyAdjacent(block))
				if(condition(adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	template <typename F>
	[[nodiscard]] BlockIndex getBlockInRangeWithCondition(const BlockIndex& index, const DistanceInBlocks& range, F&& condition)
	{
		std::stack<BlockIndex> open;
		open.push(index);
		SmallSet<BlockIndex> closed;
		while(!open.empty())
		{
			BlockIndex block = open.top();
			assert(block.exists());
			if(condition(block))
				return block;
			open.pop();
			for(const BlockIndex& adjacent : getDirectlyAdjacent(block))
				if(taxiDistance(index, adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
		}
		return BlockIndex::null();
	}
	[[nodiscard]] SmallSet<BlockIndex> collectAdjacentsInRange(const BlockIndex& index, const DistanceInBlocks& range);
	// -Designation
	[[nodiscard]] bool designation_has(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation) const;
	void designation_set(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation);
	void designation_unset(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation);
	void designation_maybeUnset(const BlockIndex& index, const FactionId& faction, const BlockDesignation& designation);
	// -Solid.
private:
	// Private method to be used by both solid_set and solidSetCuboid.
	void solid_setShared(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed);
	// Private method to be used by both solid_setNot and solidSetNotCuboid.
	void solid_setNotShared(const BlockIndex& index);
public:
	void solid_set(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed);
	void solid_setNot(const BlockIndex& index);
	void solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed);
	void solid_setNotCuboid(const Cuboid& cuboid);
	void solid_setDynamic(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed);
	void solid_setNotDynamic(const BlockIndex& index);
	[[nodiscard]] bool solid_is(const BlockIndex& index) const;
	[[nodiscard]] MaterialTypeId solid_get(const BlockIndex& index) const;
	[[nodiscard]] Mass solid_getMass(const BlockIndex& index) const;
	[[nodiscard]] MaterialTypeId solid_getHardest(const SmallSet<BlockIndex>& blocks);
	// -BlockFeature.
	void blockFeature_construct(const BlockIndex& index, const BlockFeatureTypeId& featureType, const MaterialTypeId& materialType);
	void blockFeature_hew(const BlockIndex& index, const BlockFeatureTypeId& featureType);
	void blockFeature_remove(const BlockIndex& index, const BlockFeatureTypeId& type);
	void blockFeature_removeAll(const BlockIndex& index);
	void blockFeature_lock(const BlockIndex& index, const BlockFeatureTypeId& type);
	void blockFeature_unlock(const BlockIndex& index, const BlockFeatureTypeId& type);
	void blockFeature_close(const BlockIndex& index, const BlockFeatureTypeId& type);
	void blockFeature_open(const BlockIndex& index, const BlockFeatureTypeId& type);
	void blockFeature_setTemperature(const BlockIndex& index, const Temperature& temperature);
	void blockFeature_setAll(const BlockIndex& index, BlockFeatureSet& features);
	void blockFeature_setAllMoveDynamic(const BlockIndex& index, BlockFeatureSet&& features);
	[[nodiscard]] const BlockFeature* blockFeature_atConst(const BlockIndex& index, const BlockFeatureTypeId& blockFeatueType) const;
	[[nodiscard]] BlockFeature* blockFeature_at(const BlockIndex& index, const BlockFeatureTypeId& blockFeatueType);
	[[nodiscard]] bool blockFeature_empty(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_blocksEntrance(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_canStandAbove(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_canStandIn(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_isSupport(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_canEnterFromBelow(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_canEnterFromAbove(const BlockIndex& index, const BlockIndex& from) const;
	[[nodiscard]] bool blockFeature_multiTileCanEnterAtNonZeroZOffset(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_isOpaque(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_floorIsOpaque(const BlockIndex& index) const;
	[[nodiscard]] MaterialTypeId blockFeature_getMaterialType(const BlockIndex& index) const;
	[[nodiscard]] bool blockFeature_contains(const BlockIndex& index, const BlockFeatureTypeId& blockFeatureType) const;
	[[nodiscard]] auto& blockFeature_getAll(const BlockIndex& index) const { return m_features[index]; }
	// Should probably be followed by call to removeAll.
	[[nodiscard]] auto&& blockFeature_getAllMove(const BlockIndex& index) { return std::move(m_features[index]); }
	// -Fluids
	void fluid_spawnMist(const BlockIndex& index, const FluidTypeId& fluidType, const DistanceInBlocks maxMistSpread = DistanceInBlocks::create(0));
	void fluid_clearMist(const BlockIndex& index);
	DistanceInBlocks fluid_getMistInverseDistanceToSource(const BlockIndex& block) const;
	FluidGroup* fluid_getGroup(const BlockIndex& index, const FluidTypeId& fluidType) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void fluid_add(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_add(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType);
	// To be used durring read step.
	void fluid_remove(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType);
	// To be used used durring write step.
	void fluid_removeSyncronus(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_removeAllSyncronus(const BlockIndex& index);
	// Move less dense fluids to their group's excessVolume until Config::maxBlockVolume is achieved.
	void fluid_resolveOverfull(const BlockIndex& index);
	void fluid_onBlockSetSolid(const BlockIndex& index);
	void fluid_onBlockSetNotSolid(const BlockIndex& index);
	void fluid_mistSetFluidTypeAndInverseDistance(const BlockIndex& index, const FluidTypeId& fluidType, const DistanceInBlocks& inverseDistance);
	// TODO: This could probably be resolved in a better way.
	// Exposing these two methods breaks encapusalition a bit but allows better performance from fluidGroup.
	void fluid_setAllUnstableExcept(const BlockIndex& index, const FluidTypeId& fluidType);
	// To be used by DrainQueue / FillQueue.
	void fluid_drainInternal(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_fillInternal(const BlockIndex& index, const CollisionVolume& volume, FluidGroup& fluidGroup);
	// To be used by fluid group split, so the blocks which will be in soon to be created groups can briefly have fluid without having a fluidGroup.
	// Fluid group will be set again in addBlock within the new group's constructor.
	// This prevents a problem where addBlock attempts to remove a block from a group which it has already been removed from.
	// TODO: Seems messy.
	void fluid_unsetGroupInternal(const BlockIndex& index, const FluidTypeId& fluidType);
	void fluid_maybeRecordFluidOnDeck(const BlockIndex& index);
	void fluid_maybeEraseFluidOnDeck(const BlockIndex& index);
	// To be called from FluidGroup::splitStep only.
	[[nodiscard]] bool fluid_undisolveInternal(const BlockIndex& index, FluidGroup& fluidGroup);
private:void fluid_destroyData(const BlockIndex& index, const FluidTypeId& fluidType);
	void fluid_setTotalVolume(const BlockIndex& index, const CollisionVolume& volume);
public: [[nodiscard]] bool fluid_canEnterCurrently(const BlockIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool fluid_isAdjacentToGroup(const BlockIndex& index, const FluidGroup& fluidGroup) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeCanEnter(const BlockIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeContains(const BlockIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] FluidTypeId fluid_getTypeWithMostVolume(const BlockIndex& index) const;
	[[nodiscard]] bool fluid_canEnterEver(const BlockIndex& index) const;
	[[nodiscard]] bool fluid_typeCanEnterCurrently(const BlockIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool fluid_any(const BlockIndex& index) const;
	[[nodiscard]] bool fluid_contains(const BlockIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] const std::vector<FluidData>& fluid_getAll(const BlockIndex& index) const;
	[[nodiscard]] std::vector<FluidData>& fluid_getAllSortedByDensityAscending(const BlockIndex& index);
	[[nodiscard]] CollisionVolume fluid_getTotalVolume(const BlockIndex& index) const;
	[[nodiscard]] FluidTypeId fluid_getMist(const BlockIndex& index) const;
	[[nodiscard]] std::vector<FluidData>::iterator fluid_getDataIterator(const BlockIndex& index, const FluidTypeId& fluidType);
	[[nodiscard]] const FluidData* fluid_getData(const BlockIndex& index, const FluidTypeId& fluidType) const;
	[[nodiscard]] FluidData* fluid_getData(const BlockIndex& index, const FluidTypeId& fluidType);
	// Floating
	void floating_maybeSink(const BlockIndex& index);
	void floating_maybeFloatUp(const BlockIndex& index);
	// -Fire
	void fire_maybeIgnite(const BlockIndex& index, const MaterialTypeId& materialType);
	void fire_setPointer(const BlockIndex& index, SmallMapStable<MaterialTypeId, Fire>* pointer);
	void fire_clearPointer(const BlockIndex& index);
	[[nodiscard]] bool fire_exists(const BlockIndex& index) const;
	[[nodiscard]] bool fire_existsForMaterialType(const BlockIndex& index, const MaterialTypeId& materialType) const;
	[[nodiscard]] FireStage fire_getStage(const BlockIndex& index) const;
	[[nodiscard]] Fire& fire_get(const BlockIndex& index, const MaterialTypeId& materialType);
	// -Reservations
	void reserve(const BlockIndex& index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback = nullptr);
	void unreserve(const BlockIndex& index, CanReserve& canReserve);
	void dishonorAllReservations(const BlockIndex& index);
	void setReservationDishonorCallback(const BlockIndex& index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	[[nodiscard]] bool isReserved(const BlockIndex& index, const FactionId& faction) const;
	// To be used by CanReserve::translateAndReservePositions.
	[[nodiscard]] Reservable& getReservable(const BlockIndex& index) { return *m_reservables[index]; }
	// -Actors
	void actor_recordStatic(const BlockIndex& index, const ActorIndex& actor, const CollisionVolume& volume);
	void actor_recordDynamic(const BlockIndex& index, const ActorIndex& actor, const CollisionVolume& volume);
	void actor_eraseStatic(const BlockIndex& index, const ActorIndex& actor);
	void actor_eraseDynamic(const BlockIndex& index, const ActorIndex& actor);
	void actor_setTemperature(const BlockIndex& index, const Temperature& temperature);
	void actor_updateIndex(const BlockIndex& index, const ActorIndex& oldIndex, const ActorIndex& newIndex);
	[[nodiscard]] bool actor_canStandIn(const BlockIndex& index) const;
	[[nodiscard]] bool actor_contains(const BlockIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool actor_empty(const BlockIndex& index) const;
	[[nodiscard]] FullDisplacement actor_volumeOf(const BlockIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] SmallSet<ActorIndex>& actor_getAll(const BlockIndex& index);
	[[nodiscard]] const SmallSet<ActorIndex>& actor_getAll(const BlockIndex& index) const;
	// -Items
	void item_record(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume);
	void item_recordStatic(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume);
	void item_recordDynamic(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume);
	void item_erase(const BlockIndex& index, const ItemIndex& item);
	void item_eraseDynamic(const BlockIndex& index, const ItemIndex& item);
	void item_eraseStatic(const BlockIndex& index, const ItemIndex& item);
	void item_setTemperature(const BlockIndex& index, const Temperature& temperature);
	void item_disperseAll(const BlockIndex& index);
	void item_updateIndex(const BlockIndex& index, const ItemIndex& oldIndex, const ItemIndex& newIndex);
	ItemIndex item_addGeneric(const BlockIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	//ItemIndex get(const BlockIndex& index, ItemType& itemType) const;
	[[nodiscard]] Quantity item_getCount(const BlockIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] ItemIndex item_getGeneric(const BlockIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] SmallSet<ItemIndex>& item_getAll(const BlockIndex& index);
	[[nodiscard]] const SmallSet<ItemIndex>& item_getAll(const BlockIndex& index) const;
	[[nodiscard]] bool item_hasInstalledType(const BlockIndex& index, const ItemTypeId& itemType) const;
	[[nodiscard]] bool item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(const BlockIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool item_hasContainerContainingFluidTypeCarryableBy(const BlockIndex& index, const ActorIndex& actor, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool item_empty(const BlockIndex& index) const;
	[[nodiscard]] bool item_contains(const BlockIndex& index, const ItemIndex& item) const;
	// -Plant
	void plant_create(const BlockIndex& index, const PlantSpeciesId& plantSpecies, Percent growthPercent = Percent::null());
	void plant_updateGrowingStatus(const BlockIndex& index);
	void plant_setTemperature(const BlockIndex& index, const Temperature& temperature);
	void plant_erase(const BlockIndex& index);
	void plant_set(const BlockIndex& index, const PlantIndex& plant);
	PlantIndex plant_get(const BlockIndex& index) const;
	bool plant_canGrowHereCurrently(const BlockIndex& index, const PlantSpeciesId& plantSpecies) const;
	bool plant_canGrowHereAtSomePointToday(const BlockIndex& index, const PlantSpeciesId& plantSpecies) const;
	bool plant_canGrowHereEver(const BlockIndex& index, const PlantSpeciesId& plantSpecies) const;
	bool plant_anythingCanGrowHereEver(const BlockIndex& index) const;
	bool plant_exists(const BlockIndex& index) const;
	// -Shape / Move
	void shape_addStaticVolume(const BlockIndex& index, const CollisionVolume& volume);
	void shape_removeStaticVolume(const BlockIndex& index, const CollisionVolume& volume);
	void shape_addDynamicVolume(const BlockIndex& index, const CollisionVolume& volume);
	void shape_removeDynamicVolume(const BlockIndex& index, const CollisionVolume& volume);
	[[nodiscard]] bool shape_anythingCanEnterEver(const BlockIndex& index) const;
	[[nodiscard]] bool shape_anythingCanEnterCurrently(const BlockIndex& index) const;
	[[nodiscard]] bool shape_canFitEverOrCurrentlyDynamic(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_canFitEverOrCurrentlyStatic(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_canFitEver(const BlockIndex& index, const ShapeId& shape, const Facing4& facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverFrom(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& block) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& block, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType) const;
	[[nodiscard]] Facing4 shape_canEnterEverWithAnyFacingReturnFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType) const;
	// CanEnterCurrently methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterCurrentlyWithAnyFacingReturnFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyFrom(const BlockIndex& index, const ShapeId& shape, const BlockIndex& other, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_moveTypeCanEnter(const BlockIndex& index, const MoveTypeId& moveType) const;
	[[nodiscard]] bool shape_moveTypeCanEnterFrom(const BlockIndex& index, const MoveTypeId& moveType, const BlockIndex& from) const;
	[[nodiscard]] bool shape_moveTypeCanBreath(const BlockIndex& index, const MoveTypeId& moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithFacing(const BlockIndex& index, const ShapeId& Shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] std::pair<bool, Facing4> shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const;
	[[nodiscard]] MoveCost shape_moveCostFrom(const BlockIndex& index, const MoveTypeId& moveType, const BlockIndex& from) const;
	[[nodiscard]] bool shape_canStandIn(const BlockIndex& index) const;
	[[nodiscard]] CollisionVolume shape_getDynamicVolume(const BlockIndex& index) const;
	[[nodiscard]] CollisionVolume shape_getStaticVolume(const BlockIndex& index) const;
	[[nodiscard]] Quantity shape_getQuantityOfItemWhichCouldFit(const BlockIndex& index, const ItemTypeId& itemType) const;
	[[nodiscard]] SmallSet<BlockIndex> shape_getBelowBlocksWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing) const;
	[[nodiscard]] std::pair<BlockIndex, Facing4> shape_getNearestEnterableEverBlockWithFacing(const BlockIndex& block, const ShapeId& shape, const MoveTypeId& moveType);
	[[nodiscard]] std::pair<BlockIndex, Facing4> shape_getNearestEnterableEverOrCurrentlyBlockWithFacing(const BlockIndex& block, const ShapeId& shape, const MoveTypeId& moveType);
	// -FarmField
	void farm_insert(const BlockIndex& index, const FactionId& faction, FarmField& farmField);
	void farm_remove(const BlockIndex& index, const FactionId& faction);
	void farm_designateForHarvestIfPartOfFarmField(const BlockIndex& index, const PlantIndex& plant);
	void farm_designateForGiveFluidIfPartOfFarmField(const BlockIndex& index, const PlantIndex& plant);
	void farm_maybeDesignateForSowingIfPartOfFarmField(const BlockIndex& index);
	void farm_removeAllHarvestDesignations(const BlockIndex& index);
	void farm_removeAllGiveFluidDesignations(const BlockIndex& index);
	void farm_removeAllSowSeedsDesignations(const BlockIndex& index);
	[[nodiscard]] bool farm_isSowingSeasonFor(const PlantSpeciesId& species) const;
	[[nodiscard]] bool farm_contains(const BlockIndex& index, const FactionId& faction) const;
	[[nodiscard]] FarmField* farm_get(const BlockIndex& index, const FactionId& faction);
	[[nodiscard]] const FarmField* farm_get(const BlockIndex& index, const FactionId& faction) const;
	// -StockPile
	void stockpile_recordMembership(const BlockIndex& index, StockPile& stockPile);
	void stockpile_recordNoLongerMember(const BlockIndex& index, StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void stockpile_updateActive(const BlockIndex& index);
	[[nodiscard]] StockPile* stockpile_getForFaction(const BlockIndex& index, const FactionId& faction);
	[[nodiscard]] const StockPile* stockpile_getForFaction(const BlockIndex& index, const FactionId& faction) const;
	[[nodiscard]] bool stockpile_contains(const BlockIndex& index, const FactionId& faction) const;
	[[nodiscard]] bool stockpile_isAvalible(const BlockIndex& index, const FactionId& faction) const;
	// -Project
	void project_add(const BlockIndex& index, Project& project);
	void project_remove(const BlockIndex& index, Project& project);
	Percent project_getPercentComplete(const BlockIndex& index, const FactionId& faction) const;
	[[nodiscard]] Project* project_get(const BlockIndex& index, const FactionId& faction) const;
	[[nodiscard]] Project* project_getIfBegun(const BlockIndex& index, const FactionId& faction) const;
	// -Temperature
	void temperature_freeze(const BlockIndex& index, const FluidTypeId& fluidType);
	void temperature_melt(const BlockIndex& index);
	void temperature_apply(const BlockIndex& index, const Temperature& temperature, const TemperatureDelta& delta);
	void temperature_updateDelta(const BlockIndex& index, const TemperatureDelta& delta);
	const Temperature& temperature_getAmbient(const BlockIndex& index) const;
	Temperature temperature_getDailyAverageAmbient(const BlockIndex& index) const;
	Temperature temperature_get(const BlockIndex& index) const;
	bool temperature_transmits(const BlockIndex& block) const;
	[[nodiscard]] std::string toString(const BlockIndex& index) const;
	Blocks(Blocks&) = delete;
	Blocks(Blocks&&) = delete;
};