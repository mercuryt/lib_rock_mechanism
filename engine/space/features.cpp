// TODO: change from points to cuboids in create / destroy.
#include "space.h"
#include "../area/area.h"
#include "../pointFeature.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../reference.h"
#include "../numericTypes/types.h"
#include "../definitions/plantSpecies.h"
CuboidSet Space::pointFeature_getCuboidsIntersecting(const Cuboid cuboid) const { return m_features.queryGetAllCuboids(cuboid); }
bool Space::pointFeature_contains(const Point3D point, PointFeatureTypeId pointFeatureType) const
{
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	return m_features.queryAnyWithCondition(point, condition);
}
SmallSet<std::pair<Cuboid, PointFeature>> Space::pointFeature_getAllWithCuboids(const Cuboid cuboid) const { return m_features.queryGetAllWithCuboids(cuboid); }
const PointFeature Space::pointFeature_at(const Cuboid cuboid, PointFeatureTypeId pointFeatureType) const
{
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	assert(m_features.queryCountWithCondition(cuboid, condition) < 2);
	return m_features.queryGetOneWithCondition(cuboid, condition);
}
void Space::pointFeature_remove(const Point3D point, PointFeatureTypeId pointFeatureType)
{
	assert(!solid_isAny(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	m_features.maybeRemoveWithConditionOne(point, condition);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(Cuboid::create(point, point));
	m_area.m_hasPaths.update(m_area, getAdjacentWithEdgeAndCornerAdjacent(point));
	if(!transmitedTemperaturePreviously && temperature_transmits(point))
		m_area.m_hasTemperature.onTemperatureCanNowTransmit(m_area, CuboidSet::create(point));
	m_exposedToSky.maybeSetCuboid(m_area, {point, point});
}
void Space::pointFeature_removeAll(const Point3D point)
{
	assert(!solid_isAny(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	m_features.maybeRemove(point);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(Cuboid::create(point, point));
	m_area.m_hasPaths.update(m_area, getAdjacentWithEdgeAndCornerAdjacent(point));
	if(!transmitedTemperaturePreviously && temperature_transmits(point))
		m_area.m_hasTemperature.onTemperatureCanNowTransmit(m_area, CuboidSet::create(point));
	m_exposedToSky.maybeSetCuboid(m_area, {point, point});
}
void Space::pointFeature_add(Cuboid cuboid, PointFeature feature)
{
	// Floors, floor grates, and hatches cannot overlap.
	if(feature.blocksVerticalTravel())
		assert(!m_features.queryAnyWithCondition(cuboid, [&](const PointFeature existingFeature){ return existingFeature.blocksVerticalTravelEver(); }));
	assert(!solid_isAny(cuboid));
	assert(pointFeature_empty(cuboid));
	[[maybe_unused]] Plants& plants = m_area.getPlants();
	assert(!plant_queryAnyWithCondition(cuboid, [&plants](PlantIndex plant){ return PlantSpecies::getIsTree(plants.getSpecies(plant)); }));
	plant_erase(cuboid);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(cuboid);
	m_features.insert(cuboid, feature);
	const bool materialTypeIsTransparent = MaterialType::getTransparent(feature.materialType);
	if(PointFeatureType::byId(feature.pointFeatureType).opaque && !materialTypeIsTransparent && cuboid.m_high.z() != 0)
		m_exposedToSky.maybeUnsetBeneathTopLayer(m_area, cuboid);
	m_area.m_opacityFacade.update(m_area, cuboid);
	m_area.m_hasPaths.update(m_area, cuboid.inflated({1}));
	if(feature.blocksTemperature())
		m_area.m_hasTemperature.onTemperatureCanNoLongerTransmit(m_area, CuboidSet::create(cuboid));
}
void Space::pointFeature_construct(Cuboid cuboid, PointFeatureTypeId pointFeatureType, MaterialTypeId materialType)
{
	/*
	PointFeature feature = PointFeature::create(materialType, pointFeatureType);
	pointFeature_add(cuboid, feature);
	*/
	for(Point3D point : cuboid)
	{
		assert(!solid_isAny(point));
		const bool transmitedTemperaturePreviously = temperature_transmits(point);
		const bool materialTypeIsTransparent = MaterialType::getTransparent(materialType);
		Plants& plants = m_area.getPlants();
		if(plant_exists(point))
		{
			assert(!PlantSpecies::getIsTree(plants.getSpecies(plant_get(point))));
			plant_erase(point);
		}
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(Cuboid::create(point, point));
		PointFeature feature = PointFeature::create(materialType, pointFeatureType);
		// Floors, floor grates, and hatches cannot overlap.
		if(feature.blocksVerticalTravel())
			assert(!m_features.queryAnyWithCondition(point, [&](const PointFeature existingFeature){ return existingFeature.blocksVerticalTravelEver(); }));
		m_features.insert(point, feature);
		m_area.m_opacityFacade.update(m_area, point);
		if(PointFeatureType::byId(pointFeatureType).opaque && !materialTypeIsTransparent)
		{
			if(point.z() != 0)
				m_exposedToSky.maybeUnsetBeneathTopLayer(m_area, Cuboid::create(point, point));
		}
		m_area.m_hasPaths.update(m_area, getAdjacentWithEdgeAndCornerAdjacent(point));
		if(transmitedTemperaturePreviously && !temperature_transmits(point))
			m_area.m_hasTemperature.onTemperatureCanNoLongerTransmit(m_area, CuboidSet::create(point));
	}
}
void Space::pointFeature_hew(const Point3D point, PointFeatureTypeId pointFeatureType)
{
	assert(solid_isAny(point));
	const MaterialTypeId materialType = solid_get(point);
	PointFeature feature = PointFeature::create(materialType, pointFeatureType, true);
	// Floors, floor grates, and hatches cannot overlap.
	if(feature.blocksVerticalTravel())
		assert(!m_features.queryAnyWithCondition(point, [&](const PointFeature existingFeature){ return existingFeature.blocksVerticalTravelEver(); }));
	m_features.insert(point, feature);
	// Solid_setNot will handle calling AreaHasTemperatures::onCuboidCanTransmitTemperature.
	// TODO: There is no support for hewing hatches or flaps. This is ok because those things can't be hewn. Could be fixed anyway?
	const Point3D above = point.above();
	auto actorsCopy = actor_getAll(above);
	for(const ActorIndex actor : actorsCopy)
		m_area.getActors().tryToMoveSoAsNotOccuping(actor, above);
	solid_setNot(point);
	m_area.m_opacityFacade.update(m_area, point);
	// The point has been set to transparent by solid_setNot but may need to be set to opaque again depending on the feature and material types.
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(Cuboid::create(point, point));
	m_area.m_hasPaths.update(m_area, getAdjacentWithEdgeAndCornerAdjacent(point));
}
MapWithCuboidKeys<PointFeature> Space::pointFeature_getAllWithCuboidsAndRemove(const CuboidSet& cuboids)
{
	MapWithCuboidKeys<PointFeature> output;
	for(const Cuboid cuboid : cuboids)
		// TODO: RTreeData::queryGetAndRemoveAllWithCuboids.
		for(const auto& [subCuboid, pointFeature] : m_features.queryGetAllWithCuboids(cuboid))
			output.insertOrMerge(subCuboid, pointFeature);
	m_features.maybeRemove(cuboids);
	return output;
}
void Space::pointFeature_lock(const Point3D point, PointFeatureTypeId pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	const auto action = [&](PointFeature& feature){ feature.setLocked(true); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_hasPaths.update(m_area, getAdjacentWithEdgeAndCornerAdjacent(point));

}
void Space::pointFeature_unlock(const Point3D point, PointFeatureTypeId pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	const auto action = [&](PointFeature& feature){ feature.setLocked(false); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_hasPaths.update(m_area, getAdjacentWithEdgeAndCornerAdjacent(point));
}
void Space::pointFeature_close(const Point3D point, PointFeatureTypeId pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	bool isTransparent;
	auto action = [&isTransparent](PointFeature& feature) mutable { isTransparent = MaterialType::getTransparent(feature.materialType); feature.setClosed(true); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_hasTemperature.onTemperatureCanNoLongerTransmit(m_area, CuboidSet::create(point));
	if(!isTransparent)
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(Cuboid::create(point, point));
}
void Space::pointFeature_open(const Point3D point, PointFeatureTypeId pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	bool isTransparent;
	auto action = [&isTransparent](PointFeature& feature) mutable { isTransparent = MaterialType::getTransparent(feature.materialType); feature.setClosed(false); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_hasTemperature.onTemperatureCanNowTransmit(m_area, CuboidSet::create(point));
	if(!isTransparent)
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(Cuboid::create(point, point));
}
bool Space::pointFeature_canStandIn(const Point3D point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).canStandIn; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_canStandAbove(const Point3D point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).canStandAbove; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_isSupport(const Point3D point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).isSupportAgainstCaveIn; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_canEnterFromBelow(const Point3D point) const
{
	assert(shape_anythingCanEnterEver(point));
	const auto condition = [&](const PointFeature& feature) {
		return
			feature.pointFeatureType == PointFeatureTypeId::Floor ||
			feature.pointFeatureType == PointFeatureTypeId::FloorGrate ||
			(feature.pointFeatureType == PointFeatureTypeId::Hatch && feature.isLocked());
	};
	return !m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_canEnterFromAbove([[maybe_unused]] const Point3D point, const Point3D from) const
{
	assert(shape_anythingCanEnterEver(point));
	return pointFeature_canEnterFromBelow(from);
}
bool Space::pointFeature_canEnterFromBelowAll(const CuboidSet& cuboids) const
{
	return !m_features.queryAnyWithCondition(cuboids, [&](const PointFeature feature) { return feature.blocksVerticalTravel(); });
}
bool Space::pointFeature_canEnterFromBelowAll(const Cuboid cuboid) const
{
	return !m_features.queryAnyWithCondition(cuboid, [&](const PointFeature feature) { return feature.blocksVerticalTravel(); });
}
bool Space::pointFeature_canEnterFromBelowAny(const Cuboid cuboid) const
{
	int volume = cuboid.volume();
	m_features.queryForEachWithCuboids(cuboid, [&volume, cuboid](const Cuboid featureCuboid, const PointFeature feature){
		if(feature.blocksVerticalTravel())
			volume -= cuboid.intersection(featureCuboid).volume();
	});
	assert(volume >= 0);
	return volume != 0;
}
MaterialTypeId Space::pointFeature_getMaterialType(const Point3D point, PointFeatureTypeId featureType) const
{
	const auto condition = [featureType](const PointFeature& feature){ return feature.pointFeatureType == featureType; };
	const PointFeature& feature = m_features.queryGetOneWithCondition(point, condition);
	return feature.materialType;
}
MaterialTypeId Space::pointFeature_getMaterialTypeFirst(const Point3D point) const
{
	const PointFeature& feature = m_features.queryGetFirst(point);
	if(feature.exists())
		return feature.materialType;
	else
		return MaterialTypeId::null();
}
bool Space::pointFeature_multiTileCanEnterAtNonZeroZOffset(const Point3D point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).blocksMultiTileShapesIfNotAtZeroZOffset; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_multiTileCanEnterAtNonZeroZOffset(const CuboidSet& cuboids) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).blocksMultiTileShapesIfNotAtZeroZOffset; };
	return !m_features.queryAnyWithCondition(cuboids, condition);
}
bool Space::pointFeature_isOpaque(const Point3D point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).opaque; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_floorIsOpaque(const Point3D point) const
{
	const auto condition = [&](const PointFeature& feature) {
		return(
			!MaterialType::getTransparent(feature.materialType) &&
			(
				feature.pointFeatureType == PointFeatureTypeId::Floor ||
				(
					feature.pointFeatureType == PointFeatureTypeId::Hatch &&
					feature.isClosed()
				)

			)
		);
	};
	return m_features.queryAnyWithCondition(point, condition);
}
void Space::pointFeature_removeOpaque(CuboidSet& cuboids) const
{
	m_features.queryForEachWithCuboids(cuboids, [&](const Cuboid cuboid, const PointFeature& feature){
		if(feature.blocksLineOfSight())
			cuboids.remove(cuboid);
	});
}