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
#include "../rtreeHelpers/rtreeHelpers.h"

#include "exposedToSky.h"
#include "support.h"

#include <vector>
#include <memory>

class FluidGroup;

struct FluidData
{
	struct Primitive
	{
		FluidGroupId::Primitive group;
		FluidTypeId::Primitive type;
		Density::Primitive density;
		[[nodiscard]] bool operator==(const Primitive&) const = default;
		[[nodiscard]] std::strong_ordering operator<=>(const Primitive&) const = default;
	};
	static_assert(std::is_trivial_v<Primitive>);
	//TODO: Replace pointer with index or id.
	FluidGroupId group = FluidGroupId::null();
	FluidTypeId type = FluidTypeId::null();
	Density density = Density::null();
	FluidData() = default;
	FluidData(FluidGroupId _group, FluidTypeId _type) :
		group(_group),
		type(_type),
		density(FluidType::getDensity(_type))
	{ }
	// Fluid data leafs should never overlap if they are the same fluid type.
	[[nodiscard]] std::strong_ordering operator<=>(const FluidData& other) const { return type <=> other.type; }
	[[nodiscard]] constexpr bool operator==(const FluidData& other) const { return other.group == group; }
	[[nodiscard]] bool empty() const { return type.empty(); }
	[[nodiscard]] bool exists() const { return !empty(); }
	[[nodiscard]] std::string toS() const { return "{type: " + FluidType::getName(type) + ", group: " + group.toS() + "}"; }
	void clear() { group.clear(); type.clear(); density.clear(); }
	constexpr Primitive get() const { return {group.get(), type.get(), density.get()}; }
	static FluidData create(const Primitive& data)
	{
		FluidData output;
		output.group = {data.group};
		output.type = {data.type};
		output.density = {data.density};
		return output;
	}
	static constexpr FluidData null() { return {}; }
	static constexpr Primitive nullPrimitive() { return {.group=FluidGroupId::nullPrimitive(), .type=FluidTypeId::nullPrimitive(), .density=Density::nullPrimitive()}; }
	// TODO:(optimization) density doesn't need to be serialized.
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FluidData, group, type, density);
};
using PointFeatureBase = RTreeData<PointFeature, RTreeDataConfigs::canOverlapAndMerge>;
class PointFeatureRTree final : public PointFeatureBase
{
public:
	// feature leaves can overlap only if they have different feature types, none block entrance, and only one blocks horizontal movement.
	[[nodiscard]] bool canOverlap(PointFeature a, PointFeature b) const;
};
class Space
{
	RTreeDataIndex<std::unique_ptr<Reservable>, RTreeDataConfigs::noMergeOrOverlap> m_reservables;
	RTreeData<MaterialTypeId> m_solid;
	PointFeatureRTree m_features;
	RTreeData<FluidData, RTreeDataConfigs::canOverlapAndMerge> m_fluid;
	//TODO: store these 4 as overlaping RTree.
	RTreeData<ActorIndex, RTreeDataConfigs::noMerge> m_actors;
	RTreeData<ItemIndex, RTreeDataConfigs::noMerge> m_items;
	RTreeData<PlantIndex> m_plants;
	RTreeData<CollisionVolume, RTreeDataConfig{}, 0> m_dynamicVolume;
	RTreeData<CollisionVolume, RTreeDataConfig{}, 0> m_staticVolume;
	RTreeBoolean m_unrevealed;
	RTreeBoolean m_constructed;
	RTreeBoolean m_dynamic;
	Support m_support;
	SmallMap<FactionId, RTreeData<RTreeDataWrapper<FarmField*, nullptr>>> m_farmFields;
	SmallMap<FactionId, RTreeData<RTreeDataWrapper<StockPile*, nullptr>>> m_stockPiles;
	SmallMap<FactionId, RTreeData<RTreeDataWrapper<Project*, nullptr>>> m_projects;
	Area& m_area;
public:
	PointsExposedToSky m_exposedToSky;
	const Coordinates m_pointToIndexConversionMultipliers;
	const Coordinates m_dimensions;
	int m_sizeXInChunks;
	int m_sizeXTimesYInChunks;
	//TODO: replace these with functions accessing m_dimensions.
	Distance m_sizeX;
	Distance m_sizeY;
	Distance m_sizeZ;
	DistanceWidth m_zLevelSize;
	Space(Area& area, Distance x, Distance y, Distance z);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void moveContentsTo(Point3D point, Point3D other);
	void maybeContentsFalls(Cuboid cuboid);
	void setDynamic(const auto& shape) { m_dynamic.maybeInsert(shape); }
	void unsetDynamic(const auto& shape) { m_dynamic.maybeRemove(shape); }
	void doSupportStep() { m_support.doStep(m_area); }
	void prepareRtrees();
	[[nodiscard]] int size() const { return m_dimensions.prod(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] OffsetCuboid offsetBoundry() const;
	[[nodiscard]] Point3D getCenterAtGroundLevel() const;
	[[nodiscard]] Distance getGroundLevel(Distance x, Distance y) const;
	// TODO: Return limited set.
	[[nodiscard]] Cuboid getAdjacentWithEdgeAndCornerAdjacent(Point3D point) const;
	[[nodiscard]] SmallSet<Point3D> getDirectlyAdjacent(Point3D point) const;
	[[nodiscard]] SmallSet<Point3D> getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(Point3D point) const;
	[[nodiscard]] SmallSet<Point3D> getDirectlyAdjacentOnSameZLevelOnly(Point3D point) const;
	[[nodiscard]] Cuboid getAdjacentWithEdgeOnSameZLevelOnly(Point3D point) const;
	// getNthAdjacent is not const because the point offsets are created and cached.
	[[nodiscard]] SmallSet<Point3D> getNthAdjacent(Point3D point, Distance distance);
	[[nodiscard]] bool isAdjacentToActor(Point3D point, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItem(Point3D point, ItemIndex item) const;
	[[nodiscard]] bool isConstructed(const auto& shape) const { return m_constructed.query(shape); }
	[[nodiscard]] CuboidSet queryConstructedGetCuboids(const auto& shape) const { return m_constructed.queryGetLeaves(shape); }
	[[nodiscard]] bool isDynamic(const auto& shape) const { return m_dynamic.query(shape); }
	[[nodiscard]] bool canSeeIntoFromAlways(Point3D point, Point3D other) const;
	[[nodiscard]] bool canSeeThrough(Cuboid cuboid) const;
	[[nodiscard]] bool canSeeThrough(Point3D point) const;
	[[nodiscard]] bool canSeeThroughFloor(Cuboid cuboid) const;
	[[nodiscard]] bool canSeeThroughFrom(Point3D point, Point3D other) const;
	[[nodiscard]] bool isSupport(Point3D point) const;
	[[nodiscard]] bool isExposedToSky(Point3D point) const;
	[[nodiscard]] bool isEdge(Point3D point) const;
	[[nodiscard]] bool isEdge(Cuboid cuboid) const;
	[[nodiscard]] bool hasLineOfSightTo(Point3D point, Point3D other) const;
	[[nodiscard]] Cuboid getZLevel(Distance z);
	[[nodiscard]] const auto& getSolid() const { return m_solid; }
	[[nodiscard]] const auto& getDynamic() const { return m_dynamic; }
	[[nodiscard]] const auto& getPointFeatures() const { return m_features; }
	[[nodiscard]] const Support& getSupport() const { return m_support; }
	[[nodiscard]] Support& getSupport() { return m_support; }
	[[nodiscard]] Distance getVerticalClearance(Cuboid cuboid) const;
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	[[nodiscard]] CuboidSet collectAdjacentsWithCondition(Point3D point, auto&& condition)
	{
		CuboidSet output;
		std::stack<Point3D> openList;
		openList.push(point);
		output.maybeAdd(point);
		while(!openList.empty())
		{
			Point3D current = openList.top();
			openList.pop();
			for(Point3D adjacent : getDirectlyAdjacent(current))
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
			for(Cuboid resultCuboid : result)
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
	[[nodiscard]] Point3D getPointInRangeWithCondition(Point3D point, Distance range, F&& condition)
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
			for(Point3D adjacent : getDirectlyAdjacent(current))
				if(current.taxiDistanceTo(adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
		}
		return Point3D::null();
	}
	[[nodiscard]] CuboidSet collectAdjacentsInRange(Point3D point, Distance range);
	// -Designation
	[[nodiscard]] bool designation_anyForFaction(FactionId  faction, SpaceDesignation designation) const;
	[[nodiscard]] bool designation_has(Point3D shape, FactionId  faction, SpaceDesignation designation) const;
	[[nodiscard]] bool designation_has(Cuboid shape, FactionId  faction, SpaceDesignation designation) const;
	[[nodiscard]] bool designation_has(const CuboidSet& shape, FactionId  faction, SpaceDesignation designation) const;
	[[nodiscard]] Point3D designation_hasPoint(Cuboid shape, FactionId  faction, SpaceDesignation designation) const;
	[[nodiscard]] Point3D designation_hasPoint(const CuboidSet& shape, FactionId  faction, SpaceDesignation designation) const;
	[[nodiscard]] CuboidSet designation_queryForFaction(Cuboid cuboid, FactionId  faction, SpaceDesignation designation) const;
	template<typename AreaT>
	void designation_queryForEachForFaction(const AreaT& area, const auto& shape, FactionId  faction, auto&& action) const
	{
		//TODO: use maybeGetForFaction instead of contains.
		if(area.m_spaceDesignations.contains(faction))
			area.m_spaceDesignations.getForFaction(faction).queryForEach(shape, action);
	}
	void designation_set(Point3D shape, FactionId  faction, SpaceDesignation designation);
	void designation_set(Cuboid shape, FactionId  faction, SpaceDesignation designation);
	void designation_set(const CuboidSet& shape, FactionId  faction, SpaceDesignation designation);
	void designation_unset(Point3D shape, FactionId  faction, SpaceDesignation designation);
	void designation_unset(Cuboid shape, FactionId  faction, SpaceDesignation designation);
	void designation_unset(const CuboidSet& shape, FactionId  faction, SpaceDesignation designation);
	void designation_maybeUnset(Point3D shape, FactionId  faction, SpaceDesignation designation);
	void designation_maybeUnset(Cuboid shape, FactionId  faction, SpaceDesignation designation);
	void designation_maybeUnset(const CuboidSet& shape, FactionId  faction, SpaceDesignation designation);
	// -Solid.
	void solid_set(Point3D point, MaterialTypeId materialType, bool constructed);
	void solid_setAll(const CuboidSet& cuboidSet, MaterialTypeId materialType, bool constructed);
	void solid_setNot(Point3D point) { solid_setNotCuboid({point, point}); }
	void solid_setNotAll(const CuboidSet& cuboidSet);
	void solid_setCuboid(Cuboid cuboid, MaterialTypeId materialType, bool constructed);
	void solid_setNotCuboid(Cuboid cuboid);
	void solid_setDynamic(Point3D point, MaterialTypeId materialType, bool constructed);
	void solid_setCuboidDynamic(Cuboid cuboid, MaterialTypeId materialType, bool constructed);
private:
	template<typename ShapeT>
	void solid_setNotDynamicBody(const ShapeT cuboids);
public:
	void solid_setNotDynamic(const CuboidSet& cuboids);
	void solid_setNotDynamic(Cuboid cuboid);
	void solid_prepare() { m_solid.prepare(); }
	void solid_removeOpaque(CuboidSet& cuboids) const;
	void solid_removeAllFrom(CuboidSet& cuboids) const;
	[[nodiscard]] bool solid_isAny(const auto& shape) const { return m_solid.queryAny(shape); }
	[[nodiscard]] MaterialTypeId solid_get(const auto& shape) const { assert(m_solid.queryCount(shape) <= 1); return m_solid.queryGetOne(shape); }
	[[nodiscard]] SmallSet<std::pair<Cuboid, MaterialTypeId>> solid_getAllWithCuboids(const auto& shape) const { return m_solid.queryGetAllWithCuboids(shape); }
	[[nodiscard]] MapWithCuboidKeys<MaterialTypeId> solid_getAllWithCuboidsAndRemove(const CuboidSet& cuboids);
	[[nodiscard]] Mass solid_getMass(Point3D point) const;
	[[nodiscard]] Mass solid_getMass(const CuboidSet& cuboidSet) const;
	[[nodiscard]] CuboidSet solid_queryCuboids(const auto& shape) const { return m_solid.queryGetAllCuboids(shape); }
	[[nodiscard]] CuboidSet solid_getAllCuboids() const { return m_solid.allCuboids(); }
	[[nodiscard]] CuboidSet solid_getCuboidsWithMaterialType(const CuboidSet& shape, MaterialTypeId materialtype) const;
	void solid_queryForEach(const auto& shape, auto&& action) const { return m_solid.queryForEach(shape, action); }
	void solid_queryForEachWithCuboids(const auto& shape, auto&& action) const { return m_solid.queryForEachWithCuboids(shape, action); }
	void solid_queryForEachCuboid(const auto& shape, auto&& action) const { return m_solid.queryForEachCuboid(shape, action); }
	Mass getMass(const auto& shape) const
	{
		assert(solid_isAny(shape));
		return MaterialType::getDensity(m_solid.queryGetOne(shape)) * FullDisplacement::create(Config::maxPointVolume.get());
	}
	[[nodiscard]] std::pair<MaterialTypeId, int> solid_getHardest(const CuboidSet& cuboids);
	// -PointFeature.
	void pointFeature_add(Cuboid cuboid, PointFeature feature);
	// TODO: make construct / hew / remove work with cuboids.
	void pointFeature_construct(const CuboidSet& cuboids, PointFeatureTypeId featureType, MaterialTypeId materialType) { for(Cuboid cuboid : cuboids) pointFeature_construct(cuboid, featureType, materialType); }
	void pointFeature_construct(Cuboid cuboid, PointFeatureTypeId featureType, MaterialTypeId materialType);
	void pointFeature_construct(Point3D point, PointFeatureTypeId featureType, MaterialTypeId materialType) { pointFeature_construct(Cuboid::create(point), featureType, materialType); }
	void pointFeature_hew(const CuboidSet& cuboids, PointFeatureTypeId featureType) { for(Cuboid cuboid : cuboids) pointFeature_hew(cuboid, featureType); }
	void pointFeature_hew(Cuboid cuboid, PointFeatureTypeId featureType);
	void pointFeature_hew(Point3D point, PointFeatureTypeId featureType);
	void pointFeature_remove(Point3D point, PointFeatureTypeId type);
	void pointFeature_removeAll(Point3D point);
	void pointFeature_removeAllWithCondition(const auto& shape, auto&& condition)
	{
		while(true)
		{
			auto [feature, cuboid] = m_features.queryGetOneWithCuboidAndCondition(shape, condition);
			if(feature == PointFeature::null())
				break;
			CuboidSet intersection = CuboidSet::create(cuboid).intersection(shape);
			for(Cuboid intersectionCuboid : intersection)
				for(Point3D point : intersectionCuboid)
					pointFeature_remove(point, feature.pointFeatureType);
		}
	}
	void pointFeature_removeAllWithMaterialType(const auto& shape, MaterialTypeId materialType)
	{
		pointFeature_removeAllWithCondition(shape, [materialType](PointFeature feature){ return feature.materialType == materialType; });
	}
	void pointFeature_lock(Point3D point, PointFeatureTypeId type);
	void pointFeature_unlock(Point3D point, PointFeatureTypeId type);
	void pointFeature_close(Point3D point, PointFeatureTypeId type);
	void pointFeature_open(Point3D point, PointFeatureTypeId type);
	void pointFeature_removeOpaque(CuboidSet& cuboids) const;
	void pointFeature_queryForEachWithCuboids(const auto& shape, auto&& action) const { m_features.queryForEachWithCuboids(shape, action); }
	void pointFeature_queryForEachCuboid(const auto& shape, auto&& action) const { return m_features.queryForEachCuboid(shape, action); }
	[[nodiscard]] PointFeature pointFeature_queryGetOne(const auto& shape, auto&& condition) const { return m_features.queryGetOneWithCondition(shape, condition); }
	[[nodiscard]] CuboidSet pointFeature_queryCuboids(const auto& shape) const { return m_features.queryGetAllCuboids(shape); }
	[[nodiscard]] SmallMap<PointFeature, CuboidSet> pointFeature_queryWithCuboidsCollated(const auto& shape) const { return m_fluid.queryWithCuboidsCollated(shape); }
	[[nodiscard]] MapWithCuboidKeys<PointFeature> pointFeature_getAllWithCuboidsAndRemove(const CuboidSet& cuboids);
	[[nodiscard]] const PointFeature pointFeature_at(Cuboid cuboid, PointFeatureTypeId pointFeatureType) const;
	[[nodiscard]] const PointFeature pointFeature_at(Point3D point, PointFeatureTypeId pointFeatureType) const { return pointFeature_at({point, point}, pointFeatureType); }
	[[nodiscard]] bool pointFeature_empty(const auto& shape) const { return !m_features.queryAny(shape); }
	[[nodiscard]] bool pointFeature_queryAnyWithCondition(const auto& shape, auto&& condition) const { return m_features.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] bool pointFeature_blocksEntrance(const auto& shape) const
	{
		const auto condition = [&](const PointFeature& feature){
			return PointFeatureType::byId(feature.pointFeatureType).blocksEntrance || (feature.pointFeatureType == PointFeatureTypeId::Door && feature.isLocked());
		};
		return m_features.queryAnyWithCondition(shape, condition);
	}
	[[nodiscard]] bool pointFeature_blocksEntranceBatch(const auto& shapes) const
	{
		const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == PointFeatureTypeId::Door && feature.isLocked(); };
		return m_features.batchQueryWithConditionAny(shapes, condition);
	}
	[[nodiscard]] bool pointFeature_canStandAbove(Point3D point) const;
	[[nodiscard]] bool pointFeature_canStandIn(Point3D point) const;
	[[nodiscard]] bool pointFeature_isSupport(Point3D point) const;
	[[nodiscard]] bool pointFeature_canEnterFromBelow(Point3D point) const;
	[[nodiscard]] bool pointFeature_canEnterFromAbove(Point3D point, Point3D from) const;
	[[nodiscard]] bool pointFeature_canEnterFromBelowAll(const CuboidSet& cuboids) const;
	[[nodiscard]] bool pointFeature_canEnterFromBelowAll(Cuboid cuboid) const;
	[[nodiscard]] bool pointFeature_canEnterFromBelowAny(Cuboid cuboid) const;
	[[nodiscard]] bool pointFeature_multiTileCanEnterAtNonZeroZOffset(Point3D point) const;
	[[nodiscard]] bool pointFeature_multiTileCanEnterAtNonZeroZOffset(const CuboidSet& point) const;
	[[nodiscard]] bool pointFeature_isOpaque(Point3D point) const;
	[[nodiscard]] bool pointFeature_floorIsOpaque(Point3D point) const;
	[[nodiscard]] MaterialTypeId pointFeature_getMaterialType(Point3D point, PointFeatureTypeId pointFeatureType) const;
	[[nodiscard]] MaterialTypeId pointFeature_getMaterialTypeFirst(Point3D point) const;
	[[nodiscard]] bool pointFeature_contains(Point3D point, PointFeatureTypeId pointFeatureType) const;
	[[nodiscard]] CuboidSet pointFeature_getCuboidsIntersecting(Cuboid cuboid) const;
	[[nodiscard]] CuboidSet pointFeature_queryCuboids(Cuboid cuboid, auto&& condition) const { return m_features.queryGetAllCuboidsWithCondition(cuboid, condition); }
	[[nodiscard]] SmallSet<std::pair<Cuboid, PointFeature>> pointFeature_getAllWithCuboids(Cuboid cuboid) const;
	[[nodiscard]] SmallSet<PointFeature> pointFeature_getAll(const auto& shape) const { return m_features.queryGetAll(shape); }
	[[nodiscard]] CuboidSet pointFeature_getCuboidsWithMaterialType(const CuboidSet& shape, MaterialTypeId materialtype) const;
	// -Fluids
	void fluid_add(const CuboidSet& shape, int64_t volume, FluidTypeId fluidtype);
	void fluid_remove(const CuboidSet& shape, int64_t volume, FluidTypeId fluidtype);
	// To be used by FluidGroup.
	void fluid_flowInto(const CuboidSet& cuboids, FluidGroup& group);
	void fluid_flowOutFrom(const CuboidSet& cuboid, FluidGroup& group);
	void fluid_setGroupId(const CuboidSet& shape, FluidTypeId fluidType, FluidGroupId group);
	void fluid_removeAllFilledWithDensityEqualOrGreaterThenFrom(CuboidSet& cuboids, FluidTypeId fluidType) const;
	void fluid_removeAllFrom(CuboidSet& cuboids) const { m_fluid.queryRemove(cuboids); }
	void fluid_forEach(const auto& shape, auto&& action) const { m_fluid.queryForEach(shape, action); }
	void fluid_forEachAll(auto&& action) const { m_fluid.forEach(action); }
	void fluid_forEachWithCuboid(const auto& shape, auto&& action) const { m_fluid.queryForEachWithCuboids(shape, action); }
	void fluid_forEachCuboidAll(auto&& action) const { m_fluid.forEachCuboid(action); }
	void fluid_onSetNotSolid(const CuboidSet& cuboid);
	void fluid_onSetSolid(const CuboidSet& cuboid);
	void fluid_maybeRecordFluidOnDeck(const CuboidSet& points);
	void fluid_maybeEraseFluidOnDeck(const CuboidSet& points);
	void fluid_addSource(const CuboidSet& shape, FluidTypeId type, CollisionVolume level);
	void fluid_queryForEachWithCuboids(const auto& shape, auto&& action) const { m_fluid.queryForEachWithCuboids(shape, action); }
	void fluid_queryForEach(const auto& shape, auto&& action) const { m_fluid.queryForEach(shape, action); }
	[[nodiscard]] CollisionVolume fluid_containsVolumeOfEqualOrGreaterDensity(Point3D point, FluidTypeId fluidType) const;
	[[nodiscard]] SmallSet<FluidGroup*> fluid_getGroups(const CuboidSet& shape);
	[[nodiscard]] SmallSet<FluidGroup*> fluid_getGroupsWithType(const CuboidSet& shape, FluidTypeId fluidType);
	[[nodiscard]] FluidGroup* fluid_getGroup(Point3D point, FluidTypeId fluidType) const;
	[[nodiscard]] CuboidSet fluid_getAdjacentWithConditionRecursive(const auto& shape, auto&& condition) { return RTreeHelpers::getAdjacentWithConditionRecursive<FluidData>(m_fluid, shape, condition); }
	[[nodiscard]] CollisionVolume fluid_volumeOfTypeContains(Point3D point, FluidTypeId fluidType) const;
	[[nodiscard]] bool fluid_any(const auto& shape) const { return m_fluid.queryAny(shape); }
	template<typename ShapeT>
	[[nodiscard]] bool fluid_contains(ShapeT shape, FluidTypeId fluidType) const;
	[[nodiscard]] bool fluid_contains(const CuboidSet& shape, FluidTypeId fluidType) const;
	template<typename ShapeT>
	[[nodiscard]] Point3D fluid_containsPoint(ShapeT&& shape, FluidTypeId fluidType) const;
	[[nodiscard]] SmallSet<FluidData> fluid_getAll(const auto& shape) const { return m_fluid.queryGetAll(shape); }
	[[nodiscard]] CuboidSet fluid_getAllCuboids() const { return m_fluid.allCuboids(); }
	[[nodiscard]] CuboidSet fluid_getAllCuboidsWithCondition(auto&& condition) const { return m_fluid.allCuboidsWithCondition(condition); }
	[[nodiscard]] const MapWithCuboidKeys<std::pair<FluidTypeId, int64_t>> fluid_getWithCuboidsAndRemoveAll(const CuboidSet& cuboids);
	[[nodiscard]] CollisionVolume fluid_getTotalVolume(Point3D point) const;
	[[nodiscard]] CuboidSet fluid_queryGetCuboids(const auto& shape) const { return m_fluid.queryGetAllCuboids(shape); }
	[[nodiscard]] CuboidSet fluid_queryGetCuboidsWithCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetAllCuboidsWithCondition(shape, condition); }
	[[nodiscard]] CuboidSet fluid_queryGetCuboidsWithType(const auto& shape, FluidTypeId type) const { return m_fluid.queryGetAllCuboidsWithCondition(shape, [type](FluidData fluid){ return fluid.type == type; }); }
	[[nodiscard]] Point3D fluid_queryGetPointWithCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetOnePointWithCondition(shape, condition); }
	[[nodiscard]] const SmallSet<FluidData> fluid_queryGetAll(const auto& shape) const { return m_fluid.queryGetAll(shape); }
	[[nodiscard]] const SmallSet<FluidData> fluid_queryGetWithCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetAllWithCondition(shape, condition); }
	[[nodiscard]] const std::vector<std::pair<Cuboid, FluidData>> fluid_queryGetWithCuboidsAndCondition(const auto& shape, const auto& condition) const { return m_fluid.queryGetAllWithCuboidsAndCondition(shape, condition); }
	[[nodiscard]] const SmallSet<std::pair<Cuboid, FluidData>> fluid_queryGetWithCuboids(const auto& shape) const { return m_fluid.queryGetAllWithCuboids(shape); }
	[[nodiscard]] bool fluid_queryAnyWithCondition(const auto& shape, auto&& condition) const { return m_fluid.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] bool fluid_shapeIsMostlySurroundedByFluidOfTypeAtDistanceAboveLocationWithFacing(const ShapeId shape, FluidTypeId fluidType, Distance distance, Point3D location, const Facing4 facing) const;
	[[nodiscard]] bool fluid_allPointsContainedWithCondition(const auto& shape, auto&& condition) const { return m_fluid.queryAllWithCondition(shape, condition); }
	// Floating
	void floating_maybeSink(const CuboidSet& points);
	void floating_maybeFloatUp(const CuboidSet& points);
	// -Reservations
	void reserve(Point3D shape, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback = nullptr);
	void unreserve(Point3D shape, CanReserve& canReserve);
	void dishonorAllReservations(Point3D point);
	void setReservationDishonorCallback(Point3D point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback);
	[[nodiscard]] bool isReserved(Point3D point, FactionId  faction) const;
	[[nodiscard]] bool isReservedAny(Cuboid cuboid, FactionId  faction) const;
	[[nodiscard]] bool isReservedAny(const CuboidSet& cuboids, FactionId  faction) const;
	// To be used by CanReserve::translateAndReservePositions.
	[[nodiscard]] Reservable& getReservable(Point3D point);
	void reservation_removeFromForFaction(CuboidSet& cuboids, FactionId faction) const;
	// -Actors
	void actor_recordStatic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, ActorIndex actor);
	void actor_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, ActorIndex actor);
	void actor_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, ActorIndex actor);
	void actor_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, ActorIndex actor);
	void actor_setTemperature(Point3D point, const Temperature temperature);
	void actor_updateIndex(Cuboid cuboid, ActorIndex oldIndex, ActorIndex newIndex);
	void actor_queryForEach(const auto& shape, auto&& action) const { m_actors.queryForEach(shape, action); }
	void actor_queryForEachWithCuboid(const auto& shape, auto&& action) const { m_actors.queryForEachWithCuboids(shape, action); }
	[[nodiscard]] bool actor_contains(const auto& shape, ActorIndex actor) const { return m_actors.queryAnyEqual(shape, actor); }
	[[nodiscard]] bool actor_empty(const auto& shape) const { return !m_actors.queryAny(shape); }
	[[nodiscard]] SmallSet<ActorIndex> actor_getAll(const auto& shape) const { return m_actors.queryGetAll(shape); }
	[[nodiscard]] SmallSet<ActorReference> actor_getAllReferences(auto& actors, const auto& shape) const
	{
		SmallSet<ActorReference> output;
		m_actors.queryForEach(shape, [&](ActorIndex index){
			output.insert(actors.getReference(index));
		});
		return output;
	}
	[[nodiscard]] bool actor_queryAnyWithCondition(const auto& shape, const auto& condition) { return m_actors.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] CuboidSet actor_queryCuboidsWithCondition(const auto& shape, const auto& condition) { return m_actors.queryGetAllCuboidsWithCondition(shape, condition); }
	// -Items
	void item_record(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, ItemIndex item);
	void item_recordStatic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, ItemIndex item);
	void item_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, ItemIndex item);
	void item_erase(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, ItemIndex item);
	void item_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, ItemIndex item);
	void item_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes, ItemIndex item);
	void item_disperseAll(Point3D point);
	void item_updateIndex(Cuboid cuboid, ItemIndex oldIndex, ItemIndex newIndex);
	void item_addChunksAndPiles(const CuboidSet& cuboids, CollisionVolume volume, MaterialTypeId matreialType);
	void item_queryForEach(const auto& shape, auto&& action) const { m_items.queryForEach(shape, action); }
	void item_queryForEachCuboid(const auto& shape, auto&& action) const { m_items.queryForEachCuboid(shape, action); }
	void item_queryForEachWithCuboid(const auto& shape, auto&& action) const { m_items.queryForEachWithCuboids(shape, action); }
	ItemIndex item_addGeneric(Point3D point, ItemTypeId itemType, MaterialTypeId materialType, const Quantity quantity);
	//ItemIndex get(Point3D point, ItemType& itemType) const;
	[[nodiscard]] Quantity item_getCount(Point3D point, ItemTypeId itemType, MaterialTypeId materialType) const;
	[[nodiscard]] ItemIndex item_getGeneric(Point3D point, ItemTypeId itemType, MaterialTypeId materialType) const;
	[[nodiscard]] SmallSet<ItemIndex> item_getAll(const auto& shape) const { return m_items.queryGetAll(shape); }
	[[nodiscard]] SmallSet<ItemReference> item_getAllReferences(auto& items, const auto& shape) const
	{
		SmallSet<ItemReference> output;
		m_items.queryForEach(shape, [&](ItemIndex index){
			output.insert(items.getReference(index));
		});
		return output;
	}
	[[nodiscard]] bool item_hasInstalledType(Point3D point, ItemTypeId itemType) const;
	[[nodiscard]] bool item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(Point3D point, ActorIndex actor) const;
	[[nodiscard]] bool item_hasContainerContainingFluidTypeCarryableBy(Point3D point, ActorIndex actor, FluidTypeId fluidType) const;
	[[nodiscard]] bool item_empty(const auto& shape) const { return !m_items.queryAny(shape); }
	[[nodiscard]] bool item_contains(Point3D point, ItemIndex item) const;
	[[nodiscard]] bool item_queryAnyWithCondition(const auto& shape, const auto& condition) const { return m_items.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] ItemIndex item_getOneWithCondition(const auto& shape, const auto& condition) const { return m_items.queryGetOneWithCondition(shape, condition); }
	// -Plant
	PlantIndex plant_create(Point3D point, PlantSpeciesId plantSpecies, Percent growthPercent = Percent::null());
	void plant_updateGrowingStatus(Point3D point);
	void plant_updateGrowingStatus(const CuboidSet& cuboids);
	void plant_setTemperature(Point3D point, const Temperature temperature);
	void plant_erase(const auto& shape) { m_plants.maybeRemove(shape); }
	void plant_set(const auto& shape, PlantIndex plant)
	{
		// TODO: Make plants able to overlap.
		assert(!m_plants.queryAny(shape));
		m_plants.maybeInsert(shape, plant);
	}
	void plant_queryForEachWithCuboids(const auto& shape, auto&& action) const { m_plants.queryForEachWithCuboids(shape, action); }
	void plant_queryForEach(const auto& shape, auto&& action) const { m_plants.queryForEach(shape, action); }
	void plant_queryForEachCuboid(const auto& shape, auto&& action) const { m_plants.queryForEachCuboid(shape, action); }
	void plant_updateIndex(const auto& shape, PlantIndex oldIndex, PlantIndex newIndex) { m_plants.update(shape, oldIndex, newIndex); }
	void plant_forEach(const auto& shape, auto&& action) const { m_plants.queryForEach(shape, action); }
	[[nodiscard]] PlantIndex plant_get(const auto& shape) const { return m_plants.queryGetOne(shape); }
	[[nodiscard]] SmallSet<PlantIndex> plant_getAll(const auto& shape) const { return m_plants.queryGetAll(shape); }
	[[nodiscard]] SmallSet<PlantIndex> plant_getAllTrees() const;
	[[nodiscard]] bool plant_canGrowHereCurrently(Point3D point, PlantSpeciesId plantSpecies) const;
	[[nodiscard]] bool plant_canGrowHereAtSomePointToday(Point3D point, PlantSpeciesId plantSpecies) const;
	[[nodiscard]] bool plant_canGrowHereEver(Point3D point, PlantSpeciesId plantSpecies) const;
	[[nodiscard]] bool plant_anythingCanGrowHereEver(Point3D point) const;
	[[nodiscard]] bool plant_exists(const auto& shape) const { return m_plants.queryAny(shape); }
	[[nodiscard]] bool plant_queryAnyWithCondition(const auto& shape, auto&& condition) const { return m_plants.queryAnyWithCondition(shape, condition); }
	[[nodiscard]] int plant_count(const auto& shape) const { return m_plants.queryCount(shape); }
	// -Shape / Move
	void shape_addStaticVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_removeStaticVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_addDynamicVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_removeDynamicVolume(const MapWithCuboidKeys<CollisionVolume>& cuboidsAndVolumes);
	void shape_queryStaticVolumeForEachWithCuboids(const auto& shape, auto&& action) const { m_staticVolume.queryForEachWithCuboids(shape, action); }
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
	[[nodiscard]] bool shape_canFitEverOrCurrentlyDynamic(Point3D location, const ShapeId shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canFitEverOrCurrentlyStatic(Point3D location, const ShapeId shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canFitEver(Point3D location, const ShapeId shape, const Facing4 facing) const;
	[[nodiscard]] bool shape_canFitEverWithAnyFacing(Point3D location, const ShapeId shape) const;
	[[nodiscard]] bool shape_canFitCurrentlyStatic(Point3D location, const ShapeId shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canFitCurrentlyDynamic(Point3D location, const ShapeId shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverFrom(Point3D location, const ShapeId shape, MoveTypeId moveType, Point3D from) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(Point3D location, const ShapeId shape, MoveTypeId moveType, Point3D from, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithFacing(Point3D location, const ShapeId shape, MoveTypeId moveType, const Facing4 facing) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(Point3D location, const ShapeId shape, MoveTypeId moveType) const;
	[[nodiscard]] Facing4 shape_canEnterEverWithAnyFacingReturnFacing(Point3D location, const ShapeId shape, MoveTypeId moveType) const;
	// CanEnterCurrently methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(Point3D location, const ShapeId shape, MoveTypeId moveType, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacing(Point3D location, const ShapeId shape, MoveTypeId moveType, const CuboidSet& occupied) const;
	[[nodiscard]] Facing4 shape_canEnterCurrentlyWithAnyFacingReturnFacing(Point3D location, const ShapeId shape, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(Point3D location, const ShapeId shape, MoveTypeId moveType, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithAnyFacing(Point3D location, const ShapeId shape, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyFrom(Point3D location, const ShapeId shape, Point3D other, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_canEnterCurrentlyWithFacing(Point3D location, const ShapeId shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_moveTypeCanEnter(Point3D location, MoveTypeId moveType) const;
	[[nodiscard]] bool shape_moveTypeCanEnterFrom(Point3D location, MoveTypeId moveType, Point3D from) const;
	[[nodiscard]] bool shape_moveTypeCanBreath(Point3D location, MoveTypeId moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] Facing4 shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(Point3D point, const ShapeId shape, MoveTypeId moveType, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithFacing(Point3D point, const ShapeId Shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_staticCanEnterCurrentlyWithAnyFacing(Point3D point, const ShapeId shape, const CuboidSet& occupied) const;
	[[nodiscard]] std::pair<bool, Facing4> shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(Point3D point, const ShapeId shape, const CuboidSet& occupied) const;
	// TODO: redundant?
	[[nodiscard]] bool shape_staticShapeCanEnterWithFacing(Point3D point, const ShapeId shape, const Facing4 facing, const CuboidSet& occupied) const;
	[[nodiscard]] bool shape_staticShapeCanEnterWithAnyFacing(Point3D point, const ShapeId shape, const CuboidSet& occupied) const;
	[[nodiscard]] MoveCost shape_moveCostFrom(Point3D point, MoveTypeId moveType, Point3D from) const;
	[[nodiscard]] bool shape_canStandIn(Point3D point) const;
	[[nodiscard]] CollisionVolume shape_getDynamicVolume(Point3D point) const;
	[[nodiscard]] CollisionVolume shape_getStaticVolume(Point3D point) const;
	[[nodiscard]] Quantity shape_getQuantityOfItemWhichCouldFit(Point3D point, const ItemTypeId itemType) const;
	[[nodiscard]] CuboidSet shape_getBelowPointsWithFacing(Point3D point, const ShapeId shape, const Facing4 facing) const;
	[[nodiscard]] std::pair<Point3D, Facing4> shape_getNearestEnterableEverPointWithFacing(Point3D point, const ShapeId shape, MoveTypeId moveType);
	[[nodiscard]] std::pair<Point3D, Facing4> shape_getNearestEnterableEverOrCurrentlyPointWithFacing(Point3D point, const ShapeId shape, MoveTypeId moveType);
	[[nodiscard]] bool shape_cuboidCanFitCurrentlyStatic(Cuboid cuboid, const CollisionVolume volume) const;
	[[nodiscard]] bool shape_cuboidCanFitCurrentlyDynamic(Cuboid cuboid, const CollisionVolume volume) const;
	[[nodiscard]] bool shape_queryAnyDynamic(Cuboid cuboid) const;
	void shape_queryRemoveFromDynamic(CuboidSet& cuboids) const;
	// -Movement and pathing.
	// TODO: Some methods from shape probably belong here instead.
	void move_removeUnenterableFrom(CuboidSet& cuboids) const;
	void move_removeUnenterableFrom(CuboidSet& cuboids, MoveTypeId moveType) const;
	[[nodiscard]] SmallSet<Cuboid> move_splitCuboidByPartitions(Cuboid cuboid) const;
	[[nodiscard]] CuboidSet move_queryPathable(Cuboid cuboid, MoveTypeId moveType) const;
	[[nodiscard]] bool move_cuboidCanBeEnteredFrom(Cuboid from, Cuboid to, MoveTypeId moveType) const;
	[[nodiscard]] bool move_partitionExistsBetween(Cuboid a, Cuboid b) const;
	[[nodiscard]] bool move_canSwimInAny(Cuboid cuboid, MoveTypeId moveType) const;
	[[nodiscard]] bool move_containsUnenterable(Cuboid cuboid) const;
	[[nodiscard]] bool move_containsUnenterableForMoveType(Cuboid cuboid, MoveTypeId moveType) const;
	// -FarmField
	void farm_insert(const auto& shape, FactionId  faction, FarmField& farmField) { m_farmFields.getOrCreate(faction).insert(shape, RTreeDataWrapper<FarmField*, nullptr>(&farmField)); }
	template<typename ShapeT>
	void farm_remove(ShapeT&& shape, FactionId  faction);
	void farm_designateForHarvestIfPartOfFarmField(Point3D point, PlantIndex plant);
	void farm_designateForGiveFluidIfPartOfFarmField(Point3D point, PlantIndex plant);
	void farm_maybeDesignateForSowingIfPartOfFarmField(Point3D point);
	void farm_removeAllHarvestDesignations(Point3D point);
	void farm_removeAllGiveFluidDesignations(Point3D point);
	void farm_removeAllSowSeedsDesignations(Point3D point);
	void farm_queryForEachCuboidForFaction(const auto& shape, FactionId  faction, auto&& action) const
	{
		auto found = m_farmFields.find(faction);
		if(found == m_farmFields.end())
			return;
		found->second.queryForEachCuboid(shape, action);
	}
	[[nodiscard]] bool farm_isSowingSeasonFor(PlantSpeciesId species) const;
	[[nodiscard]] bool farm_contains(Point3D point, FactionId  faction) const;
	[[nodiscard]] FarmField* farm_get(const auto& shape, FactionId  faction);
	[[nodiscard]] const FarmField* farm_get(Point3D point, FactionId  faction) const;
	// -StockPile
	void stockpile_recordMembership(Point3D point, StockPile& stockPile);
	void stockpile_recordNoLongerMember(Point3D point, StockPile& stockPile);
	void stockpile_queryForEachCuboidForFaction(const auto& shape, FactionId  faction, auto&& action) const
	{
		auto found = m_stockPiles.find(faction);
		if(found == m_stockPiles.end())
			return;
		found->second.queryForEachCuboid(shape, action);
	}
private:
	template<typename ShapeT>
	[[nodiscard]] StockPile* stockpile_getOneForFactionBody(const ShapeT shape, FactionId  faction);
public:
	[[nodiscard]] StockPile* stockpile_getOneForFaction(Point3D point, FactionId  faction);
	[[nodiscard]] const StockPile* stockpile_getOneForFaction(Point3D point, FactionId  faction) const;
	[[nodiscard]] StockPile* stockpile_getOneForFaction(Cuboid cuboid, FactionId  faction);
	[[nodiscard]] StockPile* stockpile_getOneForFaction(const CuboidSet& shape, FactionId  faction);
	[[nodiscard]] SmallSet<RTreeDataWrapper<StockPile*, nullptr>> stockpile_getAllForFaction(const auto& shape, FactionId  faction) const { return m_stockPiles[faction].queryGetAll(shape); }
	[[nodiscard]] bool stockpile_contains(Point3D point, FactionId  faction) const;
	[[nodiscard]] bool stockpile_isAvalible(Point3D point, FactionId  faction) const;
	// -Project
	void project_add(Point3D point, Project& project);
	void project_remove(Point3D point, Project& project);
	[[nodiscard]] Percent project_getPercentComplete(Point3D point, FactionId  faction) const;
	[[nodiscard]] Project* project_get(Point3D point, FactionId  faction) const;
	[[nodiscard]] Project* project_getIfBegun(Point3D point, FactionId  faction) const;
	[[nodiscard]] Project* project_queryGetOne(FactionId  faction, const auto& shape, auto&& condition) const { return m_projects[faction].queryGetOneWithCondition(shape, condition).get(); }
	void project_queryForEachWithLocation(FactionId  faction, const auto& shape, auto&& action) const
	{
		auto found = m_projects.find(faction);
		if(found == m_projects.end())
			return;
		found->second.queryForEachWithCuboids(shape, [&](Cuboid cuboid, const RTreeDataWrapper<Project*, nullptr>& project){
			assert(cuboid.volume() == 1);
			action(*project.get(), cuboid.m_high);
		});
	}
	template<typename AreaT>
	[[nodiscard]] Project* project_randomForFactionWithCondition(FactionId  faction, auto&& condition, AreaT& area) const
	{
		auto wrappedCondition = [&](const RTreeDataWrapper<Project*, nullptr>& wrappedProject) { return condition(*wrappedProject.get()); };
		return area.m_simulation.m_random.getInVector(m_projects[faction].getAllWithCondition(wrappedCondition).m_data).get();
	}
	[[nodiscard]] const auto& project_getAll() const { return m_projects; }
	// -Temperature
	void temperature_freeze(const CuboidSet& cuboids, FluidTypeId fluidType);
	void temperature_meltSolid(const CuboidSet& cuboids, MaterialTypeId materialType);
	void temperature_meltFeatures(const CuboidSet& cuboids, MaterialTypeId materialType);
	void temperature_meltItems(const CuboidSet& cuboids, MaterialTypeId materialType);
	[[nodiscard]] const Temperature temperature_getAmbient(Point3D point) const;
	[[nodiscard]] Temperature temperature_getDailyAverageAmbient(Point3D point) const;
	[[nodiscard]] Temperature temperature_get(Point3D point) const;
	[[nodiscard]] bool temperature_transmits(Point3D point) const;
	[[nodiscard]] CuboidSet temperature_queryTransmitsCuboidsIntersection(const CuboidSet& cuboids) const;
	GDB_CALLABLE std::string toS(Point3D point) const;
	// Unrevealed.
	void unrevealed_queryForEach(const auto& shape, auto&& action) const { m_unrevealed.queryForEach(shape, action); }
	Space(Space&) = delete;
	Space(Space&&) = delete;
};

inline void to_json(Json& data, const FluidData::Primitive& p)
{
	data["group"] = p.group;
	data["type"] = p.type;
}
inline void from_json(const Json& data, FluidData::Primitive& p)
{
	data["group"].get_to(p.group);
	data["type"].get_to(p.type);
}