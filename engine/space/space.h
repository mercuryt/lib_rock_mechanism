#pragma once

#include "../fire.h"
#include "../fluidType.h"
#include "../reservable.h"
#include "../area/stockpile.h"
#include "../farmFields.h"
#include "../numericTypes/types.h"
#include "../designations.h"
#include "../pointFeature.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/rtreeData.h"
#include "../dataStructures/rtreeDataIndex.h"
#include "../geometry/pointSet.h"
#include "../geometry/mapWithCuboidKeys.h"

#include "exposedToSky.h"
#include "support.h"

#include <vector>
#include <memory>

class FluidGroup;

struct FluidData
{
	struct Primitive
	{
		FluidGroup* group;
		FluidTypeId::Primitive type;
		CollisionVolume::Primitive volume;
		[[nodiscard]] bool operator==(const Primitive&) const = default;
		[[nodiscard]] std::strong_ordering operator<=>(const Primitive&) const = default;
	};
	static_assert(std::is_trivial_v<Primitive>);
	//TODO: Replace pointer with index or id.
	FluidGroup* group = nullptr;
	FluidTypeId type = FluidTypeId::null();
	CollisionVolume volume = {0};
	// Fluid data leafs should never overlap if they are the same fluid type.
	[[nodiscard]] std::strong_ordering operator<=>(const FluidData& other) const { return type <=> other.type; }
	[[nodiscard]] constexpr bool operator==(const FluidData& other) const = default;
	[[nodiscard]] bool empty() const { return type.empty() || volume == 0; }
	[[nodiscard]] bool exists() const { return !empty(); }
	[[nodiscard]] std::string toString() const { return "{type: " + FluidType::getName(type) + ", volume: " + volume.toString() + "}"; }
	void clear() { group = nullptr; type.clear(); volume = {0}; }
	constexpr Primitive get() const { return {group, type.get(), volume.get()}; }
	static FluidData create(const Primitive& data)
	{
		FluidData output;
		output.group = data.group;
		output.type = FluidTypeId::create(data.type);
		output.volume = CollisionVolume::create(data.volume);
		return output;
	}
	static constexpr FluidData null() { return {}; }
	static constexpr Primitive nullPrimitive() { return {.group=nullptr, .type=FluidTypeId::nullPrimitive(), .volume=0}; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FluidData, type, volume);
};
using FluidBase = RTreeData<FluidData, RTreeDataConfigs::canOverlapAndMerge>;
class FluidRTree final : public FluidBase
{
public:
	// m_fluid leaves can overlap only if they have different fluid types.
	[[nodiscard]] bool canOverlap(const FluidData& a, const FluidData& b) const override;
};
using PointFeatureBase = RTreeData<PointFeature, RTreeDataConfigs::canOverlapAndMerge>;
class PointFeatureRTree final : public PointFeatureBase
{
public:
	// m_fluid leaves can overlap only if they have different fluid types.
	[[nodiscard]] bool canOverlap(const PointFeature& a, const PointFeature& b) const override;
};
class Space
{
	RTreeDataIndex<std::unique_ptr<Reservable>, uint16_t, RTreeDataConfigs::noMergeOrOverlap> m_reservables;
	RTreeDataIndex<SmallMapStable<MaterialTypeId, Fire>*, uint32_t, RTreeDataConfigs::noMergeOrOverlap> m_fires;
	RTreeData<MaterialTypeId> m_solid;
	PointFeatureRTree m_features;
	FluidRTree m_fluid;
	RTreeData<FluidTypeId> m_mist;
	RTreeData<CollisionVolume, RTreeDataConfig{}, 0u> m_totalFluidVolume;
	RTreeData<DistanceSquared> m_mistInverseDistanceFromSourceSquared;
	//TODO: store these 4 as overlaping RTree.
	RTreeData<ActorIndex, RTreeDataConfigs::noMerge> m_actors;
	RTreeData<ItemIndex, RTreeDataConfigs::noMerge> m_items;
	RTreeData<PlantIndex> m_plants;
	RTreeData<CollisionVolume, RTreeDataConfig{}, 0u> m_dynamicVolume;
	RTreeData<CollisionVolume, RTreeDataConfig{}, 0u> m_staticVolume;
	RTreeData<TemperatureDelta> m_temperatureDelta;
	RTreeBoolean m_visible;
	RTreeBoolean m_constructed;
	RTreeBoolean m_dynamic;
	Support m_support;
	PointsExposedToSky m_exposedToSky;
	SmallMap<FactionId, RTreeData<RTreeDataWrapper<FarmField*, nullptr>>> m_farmFields;
	SmallMap<FactionId, RTreeData<RTreeDataWrapper<StockPile*, nullptr>>> m_stockPiles;
	SmallMap<FactionId, RTreeData<RTreeDataWrapper<Project*, nullptr>>> m_projects;
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
	void setDynamic(const auto& shape) { m_dynamic.maybeInsert(shape); }
	void unsetDynamic(const auto& shape) { m_dynamic.maybeRemove(shape); }
	void doSupportStep() { m_support.doStep(m_area); }
	void prepareRtrees();
	[[nodiscard]] uint size() const { return m_dimensions.prod(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] OffsetCuboid offsetBoundry() const;
	[[nodiscard]] Point3D getCenterAtGroundLevel() const;
	// TODO: Return limited set.
	[[nodiscard]] Cuboid getAdjacentWithEdgeAndCornerAdjacent(const Point3D& point) const;
	[[nodiscard]] SmallSet<Point3D> getDirectlyAdjacent(const Point3D& point) const;
	[[nodiscard]] SmallSet<Point3D> getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(const Point3D& point) const;
	[[nodiscard]] SmallSet<Point3D> getDirectlyAdjacentOnSameZLevelOnly(const Point3D& point) const;
	[[nodiscard]] Cuboid getAdjacentWithEdgeOnSameZLevelOnly(const Point3D& point) const;
	// getNthAdjacent is not const because the point offsets are created and cached.
	[[nodiscard]] SmallSet<Point3D> getNthAdjacent(const Point3D& point, const Distance& distance);
	[[nodiscard]] bool isAdjacentToActor(const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItem(const Point3D& point, const ItemIndex& item) const;
	[[nodiscard]] bool isConstructed(const auto& shape) const { return m_constructed.query(shape); }
	[[nodiscard]] bool isDynamic(const auto& shape) const { return m_dynamic.query(shape); }
	[[nodiscard]] bool canSeeIntoFromAlways(const Point3D& point, const Point3D& other) const;
	[[nodiscard]] bool isVisible(const auto& shape) const { return m_visible.query(shape); }
	[[nodiscard]] bool canSeeThrough(const Cuboid& cuboid) const;
	[[nodiscard]] bool canSeeThrough(const Point3D& point) const;
	[[nodiscard]] bool canSeeThroughFloor(const Cuboid& cuboid) const;
	[[nodiscard]] bool canSeeThroughFrom(const Point3D& point, const Point3D& other) const;
	[[nodiscard]] bool isSupport(const Point3D& point) const;
	[[nodiscard]] bool isExposedToSky(const Point3D& point) const;
	[[nodiscard]] bool isEdge(const Point3D& point) const;
	[[nodiscard]] bool isEdge(const Cuboid& cuboid) const;
	[[nodiscard]] bool hasLineOfSightTo(const Point3D& point, const Point3D& other) const;
	[[nodiscard]] Cuboid getZLevel(const Distance& z);
	[[nodiscard]] const auto& getSolid() const { return m_solid; }
	[[nodiscard]] const auto& getDynamic() const { return m_dynamic; }
	[[nodiscard]] const auto& getFluidTotal() const { return m_totalFluidVolume; }
	[[nodiscard]] const auto& getPointFeatures() const { return m_features; }
	[[nodiscard]] const Support& getSupport() const { return m_support; }
	[[nodiscard]] Support& getSupport() { return m_support; }
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(const Point3D& point, bool exposed);
	void setBelowVisible(const Point3D& point);
	[[nodiscard]] CuboidSet collectAdjacentsWithCondition(const Point3D& point, auto&& condition)
	{
		CuboidSet output;
		std::stack<Point3D> openList;
		openList.push(point);
		output.maybeAdd(point);
		while(!openList.empty())
		{
			Point3D current = openList.top();
			openList.pop();
			for(const Point3D& adjacent : getDirectlyAdjacent(current))
				if(condition(adjacent) && !output.contains(adjacent))
				{
					output.maybeAdd(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	template<bool includeStart = false>
	[[nodiscard]] CuboidSet collectAdjacentsWithCondition(const CuboidSet& start, auto&& condition)
	{
		CuboidSet output;
		if constexpr(includeStart)
			output = start;
		CuboidSet openList = start;
		while(!openList.empty())
		{
			Cuboid current = openList.back();
			openList.popBack();
			current.inflate({1});
			const CuboidSet result = condition(current);
			for(const Cuboid& resultCuboid : result)
				//TODO: change this from contains to containsWithinOneCuboid.
				if(!output.contains(resultCuboid))
				{
					openList.maybeAdd(resultCuboid);
					output.maybeAdd(resultCuboid);
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
			Point3D current = open.top();
			if(condition(current))
				return current;
			open.pop();
			for(const Point3D& adjacent : getDirectlyAdjacent(current))
				if(current.taxiDistanceTo(adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
		}
		return Point3D::null();
	}
	[[nodiscard]] CuboidSet collectAdjacentsInRange(const Point3D& point, const Distance& range);
	// -Designation
	[[nodiscard]] bool designation_anyForFaction(const FactionId& faction, const SpaceDesignation& designation) const;
	[[nodiscard]] bool designation_has(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation) const;
	[[nodiscard]] bool designation_has(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) const;
	[[nodiscard]] bool designation_has(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) const;
	[[nodiscard]] Point3D designation_hasPoint(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation) const;
	[[nodiscard]] Point3D designation_hasPoint(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation) const;
	void designation_set(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_set(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_set(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_unset(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_unset(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_unset(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_maybeUnset(const Point3D& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_maybeUnset(const Cuboid& shape, const FactionId& faction, const SpaceDesignation& designation);
	void designation_maybeUnset(const CuboidSet& shape, const FactionId& faction, const SpaceDesignation& designation);
	// -Solid.
private:
	void solid_setShared(const Point3D& point, const MaterialTypeId& materialType, bool wasEmpty);
public:
	void solid_set(const Point3D& point, const MaterialTypeId& materialType, bool constructed);
	void solid_setNot(const Point3D& point) { solid_setNotCuboid({point, point}); }
	void solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed);
	void solid_setNotCuboid(const Cuboid& cuboid);
	void solid_setDynamic(const Point3D& point, const MaterialTypeId& materialType, bool constructed);
	void solid_setCuboidDynamic(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed);
	void solid_setNotDynamic(const Point3D& point);
	void solid_setCuboidNotDynamic(const Cuboid& cuboid);
	void solid_prepare() { m_solid.prepare(); }
	[[nodiscard]] bool solid_isAny(const auto& shape) const { return m_solid.queryAny(shape); }
	[[nodiscard]] MaterialTypeId solid_get(const auto& shape) const { assert(m_solid.queryCount(shape) <= 1); return m_solid.queryGetOne(shape); }
	[[nodiscard]] MapWithCuboidKeys<MaterialTypeId> solid_getAllWithCuboids(const auto& shape) const { return m_solid.queryGetAllWithCuboids(shape); }
	[[nodiscard]] MapWithCuboidKeys<MaterialTypeId> solid_getAllWithCuboidsAndRemove(const CuboidSet& cuboids);
	[[nodiscard]] Mass solid_getMass(const Point3D& point) const;
	[[nodiscard]] Mass solid_getMass(const CuboidSet& cuboidSet) const;
	[[nodiscard]] CuboidSet solid_queryCuboids(const auto& shape) const { return m_solid.queryGetAllCuboids(shape); }
	Mass getMass(const auto& shape) const
	{
		assert(solid_isAny(shape));
		return MaterialType::getDensity(m_solid.queryGetOne(shape)) * FullDisplacement::create(Config::maxPointVolume.get());
	}
	[[nodiscard]] std::pair<MaterialTypeId, uint32_t> solid_getHardest(const CuboidSet& cuboids);
	// -PointFeature.
	void pointFeature_add(const Cuboid& cuboid, const PointFeature& feature);
	// TODO: remove pointFeature_add, use cuboid instead.
	void pointFeature_add(const Point3D& point, const PointFeature& feature);
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
	[[nodiscard]] MapWithCuboidKeys<PointFeature> pointFeature_getAllWithCuboidsAndRemove(const CuboidSet& cuboids);
	[[nodiscard]] const PointFeature pointFeature_at(const Cuboid& cuboid, const PointFeatureTypeId& pointFeatureType) const;
	[[nodiscard]] const PointFeature pointFeature_at(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const { return pointFeature_at({point, point}, pointFeatureType); }
	[[nodiscard]] bool pointFeature_empty(const auto& shape) const { return !m_features.queryAny(shape); }
	[[nodiscard]] bool pointFeature_blocksEntrance(const auto& shape) const
	{
		const auto condition  = [&](const PointFeature& feature){
			return PointFeatureType::byId(feature.pointFeatureType).blocksEntrance || (feature.pointFeatureType == PointFeatureTypeId::Door && feature.isLocked());
		};
		return m_features.queryAnyWithCondition(shape, condition);
	}
	[[nodiscard]] bool pointFeature_blocksEntranceBatch(const auto& shapes) const
	{
		const auto condition  = [&](const PointFeature& feature){ return feature.pointFeatureType == PointFeatureTypeId::Door && feature.isLocked(); };
		return m_features.batchQueryWithConditionAny(shapes, condition);
	}
	[[nodiscard]] bool pointFeature_canStandAbove(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canStandIn(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_isSupport(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canEnterFromBelow(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_canEnterFromAbove(const Point3D& point, const Point3D& from) const;
	[[nodiscard]] bool pointFeature_multiTileCanEnterAtNonZeroZOffset(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_isOpaque(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_floorIsOpaque(const Point3D& point) const;
	[[nodiscard]] MaterialTypeId pointFeature_getMaterialType(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const;
	[[nodiscard]] MaterialTypeId pointFeature_getMaterialTypeFirst(const Point3D& point) const;
	[[nodiscard]] bool pointFeature_contains(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const;
	[[nodiscard]] CuboidSet pointFeature_getCuboidsIntersecting(const Cuboid& cuboid) const;
	[[nodiscard]] CuboidSet pointFeature_queryCuboids(const Cuboid& cuboid, auto&& condition) const { return m_features.queryGetAllCuboidsWithCondition(cuboid, condition); }
	[[nodiscard]] SmallSet<std::pair<Cuboid, PointFeature>> pointFeature_getAllWithCuboids(const Cuboid& cuboid) const;
	// -Fluids
	void fluid_spawnMist(const Point3D& point, const FluidTypeId& fluidType, const Distance maxMistSpread = Distance::create(0));
	void fluid_clearMist(const Point3D& point);
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void fluid_add(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_add(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_add(const CuboidSet& points, const CollisionVolume& volume, const FluidTypeId& fluidType);
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
	void fluid_setAllUnstableExcept(const CuboidSet& points, const FluidTypeId& fluidType);
	// To be used by DrainQueue / FillQueue.
	void fluid_drainInternal(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void fluid_fillInternal(const Cuboid& cuboid, const CollisionVolume& volume, FluidGroup& fluidGroup);
	// To be used by fluid group split, so the space which will be in soon to be created groups can briefly have fluid without having a fluidGroup.
	// Fluid group will be set again in addPoint within the new group's constructor.
	// This prevents a problem where addPoint attempts to remove a point from a group which it has already been removed from.
	// TODO: Seems messy.
	void fluid_unsetGroupsInternal(const CuboidSet& points, const FluidTypeId& fluidType);
	void fluid_setGroupsInternal(const CuboidSet& points, const FluidTypeId& fluidType, FluidGroup& fluidGroup);
	void fluid_maybeRecordFluidOnDeck(const CuboidSet& points);
	void fluid_maybeEraseFluidOnDeck(const CuboidSet& points);
	void fluid_removePointsWhichCannotBeEnteredEverFromCuboidSet(CuboidSet& set) const;
	[[nodiscard]] Distance fluid_getMistInverseDistanceToSource(const Point3D& point) const;
	[[nodiscard]] FluidGroup* fluid_getGroup(const Point3D& point, const FluidTypeId& fluidType) const;
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
	[[nodiscard]] bool fluid_any(const auto& shape) const { return m_fluid.queryAny(shape); }
	template<typename ShapeT>
	[[nodiscard]] bool fluid_contains(const ShapeT& shape, const FluidTypeId& fluidType) const;
	template<typename ShapeT>
	[[nodiscard]] Point3D fluid_containsPoint(const ShapeT& shape, const FluidTypeId& fluidType) const;
	[[nodiscard]] const SmallSet<FluidData> fluid_getAll(const auto& shape) const { return m_fluid.queryGetAll(shape); }
	[[nodiscard]] const SmallSet<FluidData> fluid_getAllSortedByDensityAscending(const Point3D& point);
	[[nodiscard]] const MapWithCuboidKeys<std::pair<FluidTypeId, CollisionVolume>> fluid_getWithCuboidsAndRemoveAll(const CuboidSet& cuboids);
	[[nodiscard]] CollisionVolume fluid_getTotalVolume(const Point3D& point) const;
	[[nodiscard]] FluidTypeId fluid_getMist(const Point3D& point) const;
	[[nodiscard]] const FluidData fluid_getData(const Point3D& point, const FluidTypeId& fluidType) const;
	[[nodiscard]] CuboidSet fluid_queryGetCuboids(const Cuboid& shape) const;
	[[nodiscard]] CuboidSet fluid_queryGetCuboidsWithCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetAllCuboidsWithCondition(shape, condition); }
	[[nodiscard]] Point3D fluid_queryGetPointWithCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetOnePointWithCondition(shape, condition); }
	[[nodiscard]] const SmallSet<FluidData> fluid_queryGetAll(const auto& shape) const { return m_fluid.queryGetAll(shape); }
	[[nodiscard]] const SmallSet<FluidData> fluid_queryGetWithCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetAllWithCondition(shape, condition); }
	[[nodiscard]] const std::vector<std::pair<Cuboid, FluidData>> fluid_queryGetWithCuboidsAndCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetAllWithCuboidsAndCondition(shape, condition); }
	[[nodiscard]] const SmallSet<std::pair<Cuboid, FluidData>> fluid_queryGetWithCuboids(const auto& shape) const { return m_fluid.queryGetAllWithCuboids(shape); }
	[[nodiscard]] bool fluid_queryAnyWithCondition(const auto& shape, auto&& condition) const { return m_fluid.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] bool fluid_shapeIsMostlySurroundedByFluidOfTypeAtDistanceAboveLocationWithFacing(const ShapeId& shape, const FluidTypeId& fluidType, const Distance& distance, const Point3D& location, const Facing4& facing) const;
	void fluid_forEach(const auto& shape, auto&& action) const { m_fluid.queryForEach(shape, action); }
	void fluid_forEachWithCuboid(const auto& shape, auto&& action) const { m_fluid.queryForEachWithCuboids(shape, action); }
	[[nodiscard]] CuboidSet fluid_queryGetCuboidsOverfull(const auto& shape) const
	{
		return m_totalFluidVolume.queryGetAllCuboidsWithCondition(shape, [&](const CollisionVolume& data) { return data > Config::maxPointVolume; });
	}
	__attribute__((noinline)) void fluid_validateTotalForPoint(const Point3D& point) const;
	__attribute__((noinline)) void fluid_validateAllTotals() const;
	// Floating
	void floating_maybeSink(const CuboidSet& points);
	void floating_maybeFloatUp(const CuboidSet& points);
	// -Fire
	void fire_maybeIgnite(const Point3D& point, const MaterialTypeId& materialType);
	void fire_setPointer(const Point3D& point, SmallMapStable<MaterialTypeId, Fire>* pointer);
	void fire_clearPointer(const Point3D& point);
	[[nodiscard]] bool fire_exists(const Point3D& point) const;
	[[nodiscard]] bool fire_existsForMaterialType(const Point3D& point, const MaterialTypeId& materialType) const;
	[[nodiscard]] FireStage fire_getStage(const Point3D& point) const;
	[[nodiscard]] Fire& fire_get(const Point3D& point, const MaterialTypeId& materialType);
	// -Reservations
	void reserve(const Point3D& shape, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback = nullptr);
	void unreserve(const Point3D& shape, CanReserve& canReserve);
	void dishonorAllReservations(const Point3D& point);
	void setReservationDishonorCallback(const Point3D& point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	[[nodiscard]] bool isReserved(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] bool isReservedAny(const Cuboid& cuboid, const FactionId& faction) const;
	[[nodiscard]] bool isReservedAny(const CuboidSet& cuboids, const FactionId& faction) const;
	// To be used by CanReserve::translateAndReservePositions.
	[[nodiscard]] Reservable& getReservable(const Point3D& point);
	// -Actors
	void actor_recordStatic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor);
	void actor_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor);
	void actor_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor);
	void actor_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor);
	void actor_setTemperature(const Point3D& point, const Temperature& temperature);
	void actor_updateIndex(const Cuboid& cuboid, const ActorIndex& oldIndex, const ActorIndex& newIndex);
	[[nodiscard]] bool actor_contains(const auto& shape, const ActorIndex& actor) const { return m_actors.queryAnyEqual(shape, actor); }
	[[nodiscard]] bool actor_empty(const auto& shape) const { return !m_actors.queryAny(shape); }
	[[nodiscard]] const SmallSet<ActorIndex> actor_getAll(const auto& shape) const { return m_actors.queryGetAll(shape); }
	[[nodiscard]] bool actor_queryAnyWithCondition(const auto& shape, const auto& condition) { return m_actors.queryAnyWithCondition(shape, condition); }
	// -Items
	void item_record(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, const ItemIndex& item);
	void item_recordStatic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, const ItemIndex& item);
	void item_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, const ItemIndex& item);
	void item_erase(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, const ItemIndex& item);
	void item_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, const ItemIndex& item);
	void item_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, const ItemIndex& item);
	void item_setTemperature(const Point3D& point, const Temperature& temperature);
	void item_disperseAll(const Point3D& point);
	void item_updateIndex(const Cuboid& cuboid, const ItemIndex& oldIndex, const ItemIndex& newIndex);
	[[nodiscard]] ItemIndex item_addGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	//ItemIndex get(const Point3D& point, ItemType& itemType) const;
	[[nodiscard]] Quantity item_getCount(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] ItemIndex item_getGeneric(const Point3D& point, const ItemTypeId& itemType, const MaterialTypeId& materialType) const;
	[[nodiscard]] const SmallSet<ItemIndex> item_getAll(const auto& shape) const { return m_items.queryGetAll(shape); }
	[[nodiscard]] bool item_hasInstalledType(const Point3D& point, const ItemTypeId& itemType) const;
	[[nodiscard]] bool item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool item_hasContainerContainingFluidTypeCarryableBy(const Point3D& point, const ActorIndex& actor, const FluidTypeId& fluidType) const;
	[[nodiscard]] bool item_empty(const auto& shape) const { return !m_items.queryAny(shape); }
	[[nodiscard]] bool item_contains(const Point3D& point, const ItemIndex& item) const;
	[[nodiscard]] bool item_queryAnyWithCondition(const auto& shape, const auto& condition) { return m_items.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] ItemIndex item_getOneWithCondition(const auto& shape, const auto& condition) { return m_items.queryGetOneWithCondition(shape, condition); }
	// -Plant
	PlantIndex plant_create(const Point3D& point, const PlantSpeciesId& plantSpecies, Percent growthPercent = Percent::null());
	void plant_updateGrowingStatus(const Point3D& point);
	void plant_setTemperature(const Point3D& point, const Temperature& temperature);
	void plant_erase(const auto& shape)
	{
		assert(m_plants.queryAny(shape));
		m_plants.maybeRemove(shape);
	}
	void plant_set(const auto& shape, const PlantIndex& plant)
	{
		// TODO: Make plants able to overlap.
		assert(!m_plants.queryAny(shape));
		m_plants.maybeInsert(shape, plant);
	}
	[[nodiscard]] PlantIndex plant_get(const auto& shape) const { return m_plants.queryGetOne(shape); }
	[[nodiscard]] SmallSet<PlantIndex> plant_getAll(const auto& shape) const { return m_plants.queryGetAll(shape); }
	[[nodiscard]] bool plant_canGrowHereCurrently(const Point3D& point, const PlantSpeciesId& plantSpecies) const;
	[[nodiscard]] bool plant_canGrowHereAtSomePointToday(const Point3D& point, const PlantSpeciesId& plantSpecies) const;
	[[nodiscard]] bool plant_canGrowHereEver(const Point3D& point, const PlantSpeciesId& plantSpecies) const;
	[[nodiscard]] bool plant_anythingCanGrowHereEver(const Point3D& point) const;
	[[nodiscard]] bool plant_exists(const auto& shape) const { return m_plants.queryAny(shape); }
	void plant_updateIndex(const auto& shape, const PlantIndex& oldIndex, const PlantIndex& newIndex) { m_plants.update(shape, oldIndex, newIndex); }
	// -Shape / Move
	void shape_addStaticVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_removeStaticVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_addDynamicVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_removeDynamicVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	[[nodiscard]] bool shape_anythingCanEnterEver(const auto& shape) const
	{
		if(m_dynamic.query(shape))
			return true;
		if(solid_isAny(shape))
			return false;
		return !pointFeature_blocksEntrance(shape);
	}
	[[nodiscard]] bool shape_anythingCanEnterCurrently(const auto& shape) const
	{
		assert(shape_anythingCanEnterEver(shape));
		return !m_dynamic.query(shape);
	}
	[[nodiscard]] bool shape_canFitEverOrCurrentlyDynamic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canFitEverOrCurrentlyStatic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canFitEver(const Point3D& location, const ShapeId& shape, const Facing4& facing) const;
	[[nodiscard]] bool shape_canFitCurrentlyStatic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canFitCurrentlyDynamic(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverFrom(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const Point3D& from, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType) const;
	[[nodiscard]] Facing4 shape_canEnterEverWithAnyFacingReturnFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType) const;
	// CanEnterCurrently methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const CuboidSet& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& location, const ShapeId& shape, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(const Point3D& location, const ShapeId& shape, const MoveTypeId& moveType, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithAnyFacing(const Point3D& location, const ShapeId& shape, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyFrom(const Point3D& location, const ShapeId& shape, const Point3D& other, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithFacing(const Point3D& location, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_moveTypeCanEnter(const Point3D& location, const MoveTypeId& moveType) const;
	[[nodiscard]] bool shape_moveTypeCanEnterFrom(const Point3D& location, const MoveTypeId& moveType, const Point3D& from) const;
	[[nodiscard]] bool shape_moveTypeCanBreath(const Point3D& location, const MoveTypeId& moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithFacing(const Point3D& point, const ShapeId& Shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithAnyFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const;
	[[nodiscard]] std::pair<bool, Facing4> shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const;
	// TODO: redundant?
	[[nodiscard]] bool shape_staticShapeCanEnterWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithAnyFacing(const Point3D& point, const ShapeId& shape, const CuboidSet& occupied) const;
	[[nodiscard]] MoveCost shape_moveCostFrom(const Point3D& point, const MoveTypeId& moveType, const Point3D& from) const;
	[[nodiscard]] bool shape_canStandIn(const Point3D& point) const;
	[[nodiscard]] CollisionVolume shape_getDynamicVolume(const Point3D& point) const;
	[[nodiscard]] CollisionVolume shape_getStaticVolume(const Point3D& point) const;
	[[nodiscard]] Quantity shape_getQuantityOfItemWhichCouldFit(const Point3D& point, const ItemTypeId& itemType) const;
	[[nodiscard]] CuboidSet shape_getBelowPointsWithFacing(const Point3D& point, const ShapeId& shape, const Facing4& facing) const;
	[[nodiscard]] std::pair<Point3D, Facing4> shape_getNearestEnterableEverPointWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType);
	[[nodiscard]] std::pair<Point3D, Facing4> shape_getNearestEnterableEverOrCurrentlyPointWithFacing(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType);
	[[nodiscard]] bool shape_cuboidCanFitCurrentlyStatic(const Cuboid& cuboid, const CollisionVolume& volume) const;
	[[nodiscard]] bool shape_cuboidCanFitCurrentlyDynamic(const Cuboid& cuboid, const CollisionVolume& volume) const;
	// -FarmField
	void farm_insert(const auto& shape, const FactionId& faction, FarmField& farmField) { m_farmFields.getOrCreate(faction).insert(shape, RTreeDataWrapper<FarmField*, nullptr>(&farmField)); }
	template<typename ShapeT>
	void farm_remove(const ShapeT& shape, const FactionId& faction);
	void farm_designateForHarvestIfPartOfFarmField(const Point3D& point, const PlantIndex& plant);
	void farm_designateForGiveFluidIfPartOfFarmField(const Point3D& point, const PlantIndex& plant);
	void farm_maybeDesignateForSowingIfPartOfFarmField(const Point3D& point);
	void farm_removeAllHarvestDesignations(const Point3D& point);
	void farm_removeAllGiveFluidDesignations(const Point3D& point);
	void farm_removeAllSowSeedsDesignations(const Point3D& point);
	[[nodiscard]] bool farm_isSowingSeasonFor(const PlantSpeciesId& species) const;
	[[nodiscard]] bool farm_contains(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] FarmField* farm_get(const auto& shape, const FactionId& faction) { return m_farmFields[faction].queryGetOne(shape).get(); }
	[[nodiscard]] const FarmField* farm_get(const Point3D& point, const FactionId& faction) const;
	// -StockPile
	void stockpile_recordMembership(const Point3D& point, StockPile& stockPile);
	void stockpile_recordNoLongerMember(const Point3D& point, StockPile& stockPile);
	template<typename ShapeT>
	[[nodiscard]] StockPile* stockpile_getOneForFaction(const ShapeT& shape, const FactionId& faction);
	template<typename ShapeT>
	[[nodiscard]] const StockPile* stockpile_getOneForFaction(const ShapeT& shape, const FactionId& faction) const;
	[[nodiscard]] SmallSet<RTreeDataWrapper<StockPile*, nullptr>> stockpile_getAllForFaction(const auto& shape, const FactionId& faction) { return m_stockPiles[faction].queryGetAll(shape); }
	[[nodiscard]] bool stockpile_contains(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] bool stockpile_isAvalible(const Point3D& point, const FactionId& faction) const;
	// -Project
	void project_add(const Point3D& point, Project& project);
	void project_remove(const Point3D& point, Project& project);
	[[nodiscard]] Percent project_getPercentComplete(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] Project* project_get(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] Project* project_getIfBegun(const Point3D& point, const FactionId& faction) const;
	[[nodiscard]] Project* project_queryGetOne(const FactionId& faction, const auto& shape, auto&& condition) const { return m_projects[faction].queryGetOneWithCondition(shape, condition).get(); }
	// -Temperature
	void temperature_freeze(const Point3D& point, const FluidTypeId& fluidType);
	void temperature_melt(const Point3D& point);
	void temperature_apply(const Point3D& point, const Temperature& temperature, const TemperatureDelta& delta);
	void temperature_updateDelta(const Point3D& point, const TemperatureDelta& delta);
	[[nodiscard]] const Temperature& temperature_getAmbient(const Point3D& point) const;
	[[nodiscard]] Temperature temperature_getDailyAverageAmbient(const Point3D& point) const;
	[[nodiscard]] Temperature temperature_get(const Point3D& point) const;
	[[nodiscard]] bool temperature_transmits(const Point3D& point) const;
	[[nodiscard]] std::string toString(const Point3D& point) const;
	Space(Space&) = delete;
	Space(Space&&) = delete;
};

inline void to_json(Json& data, const FluidData::Primitive& p)
{
	data["group"] = (uintptr_t)p.group;
	data["type"] = p.type;
	data["volume"] = p.volume;
}
inline void from_json(const Json& data, FluidData::Primitive& p)
{
	p.group = (FluidGroup*)data["group"].get<uintptr_t>();
	data["type"].get_to(p.type);
	data["volume"].get_to(p.volume);
}