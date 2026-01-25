#include "space.h"
#include "../area/area.h"
#include "../pointFeature.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../reference.h"
#include "../numericTypes/types.h"
#include "../definitions/plantSpecies.h"
CuboidSet Space::pointFeature_getCuboidsIntersecting(const Cuboid& cuboid) const { return m_features.queryGetAllCuboids(cuboid); }
bool Space::pointFeature_contains(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const
{
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	return m_features.queryAnyWithCondition(point, condition);
}
SmallSet<std::pair<Cuboid, PointFeature>> Space::pointFeature_getAllWithCuboids(const Cuboid& cuboid) const { return m_features.queryGetAllWithCuboids(cuboid); }
const PointFeature Space::pointFeature_at(const Cuboid& cuboid, const PointFeatureTypeId& pointFeatureType) const
{
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	assert(m_features.queryCountWithCondition(cuboid, condition) < 2);
	return m_features.queryGetOneWithCondition(cuboid, condition);
}
void Space::pointFeature_remove(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(!solid_isAny(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	const bool wasOpaque = pointFeature_isOpaque(point);
	const bool floorWasOpaque = pointFeature_floorIsOpaque(point);
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	m_features.maybeRemoveWithConditionOne(point, condition);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
	if(!transmitedTemperaturePreviously && temperature_transmits(point))
		m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	if(wasOpaque && !pointFeature_isOpaque(point))
		m_area.m_visionCuboids.pointIsTransparent(point);
	if(floorWasOpaque && !pointFeature_floorIsOpaque(point))
		m_area.m_visionCuboids.pointFloorIsTransparent(point);
}
void Space::pointFeature_removeAll(const Point3D& point)
{
	assert(!solid_isAny(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	const bool wasOpaque = pointFeature_isOpaque(point);
	const bool floorWasOpaque = pointFeature_floorIsOpaque(point);
	m_features.maybeRemove(point);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
	if(!transmitedTemperaturePreviously && temperature_transmits(point))
		m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	if(wasOpaque)
		m_area.m_visionCuboids.pointIsTransparent(point);
	if(floorWasOpaque)
		m_area.m_visionCuboids.pointFloorIsTransparent(point);
}
void Space::pointFeature_add(const Cuboid& cuboid, const PointFeature& feature)
{
	for(const Point3D& point : cuboid)
		pointFeature_add(point, feature);
}
void Space::pointFeature_add(const Point3D& point, const PointFeature& feature)
{
	assert(!solid_isAny(point));
	assert(pointFeature_empty(point));
	Plants& plants = m_area.getPlants();
	if(plant_exists(point))
	{
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant_get(point))));
		plant_erase(point);
	}
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_features.insert(point, feature);
	const bool isFloorOrHatch = (feature.pointFeatureType == PointFeatureTypeId::Floor || feature.pointFeatureType == PointFeatureTypeId::Hatch);
	const bool materialTypeIsTransparent = MaterialType::getTransparent(feature.materialType);
	if(isFloorOrHatch && !materialTypeIsTransparent)
	{
		m_area.m_visionCuboids.pointFloorIsOpaque(point);
		if(point.z() != 0)
			m_exposedToSky.unset(m_area, point.below());
	}
	else if(PointFeatureType::byId(feature.pointFeatureType).opaque && !materialTypeIsTransparent)
	{
		m_area.m_visionCuboids.pointIsOpaque(point);
		m_exposedToSky.unset(m_area, point);
	}
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
	if(!temperature_transmits(point))
		m_area.m_exteriorPortals.onCuboidCanNotTransmitTemperature(m_area, {point, point});
}
void Space::pointFeature_construct(const Point3D& point, const PointFeatureTypeId& pointFeatureType, const MaterialTypeId& materialType)
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
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_features.insert(point, PointFeature::create(materialType, pointFeatureType));
	const bool isFloorOrHatch = (pointFeatureType == PointFeatureTypeId::Floor || pointFeatureType == PointFeatureTypeId::Hatch);
	if(isFloorOrHatch && !MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.pointFloorIsOpaque(point);
		m_exposedToSky.unset(m_area, point);
	}
	else if(PointFeatureType::byId(pointFeatureType).opaque && !materialTypeIsTransparent)
	{
		m_area.m_visionCuboids.pointIsOpaque(point);
		m_exposedToSky.unset(m_area, point);
	}
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
	if(transmitedTemperaturePreviously && !temperature_transmits(point))
		m_area.m_exteriorPortals.onCuboidCanNotTransmitTemperature(m_area, {point, point});
}
void Space::pointFeature_hew(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(solid_isAny(point));
	const MaterialTypeId& materialType = solid_get(point);
	const bool materialTypeIsTransparent = MaterialType::getTransparent(materialType);
	m_features.insert(point, PointFeature::create(materialType, pointFeatureType, true));
	// Solid_setNot will handle calling m_exteriorPortals.onPointCanTransmitTemperature.
	// TODO: There is no support for hewing hatches or flaps. This is ok because those things can't be hewn. Could be fixed anyway?
	const Point3D& above = point.above();
	auto actorsCopy = actor_getAll(above);
	for(const ActorIndex& actor : actorsCopy)
		m_area.getActors().tryToMoveSoAsNotOccuping(actor, above);
	solid_setNot(point);
	m_area.m_opacityFacade.update(m_area, point);
	// The point has been set to transparent by solid_setNot but may need to be set to opaque again depending on the feature  and material types.
	if(PointFeatureType::byId(pointFeatureType).opaque && !materialTypeIsTransparent)
		// Neighter floor nor hatch can be hewn so we don't need to check if this is point opaque or floor opaque.
		m_area.m_visionCuboids.pointIsOpaque(point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
}
void Space::pointFeature_setTemperature(const Point3D& point, const Temperature& temperature)
{
	for(const PointFeature& feature : m_features.queryGetAll(point))
	{
		if(MaterialType::getIgnitionTemperature(feature.materialType).exists() &&
			temperature > MaterialType::getIgnitionTemperature(feature.materialType)
		)
			m_area.m_fires.ignite(m_area, point, feature.materialType);
	}
}
MapWithCuboidKeys<PointFeature> Space::pointFeature_getAllWithCuboidsAndRemove(const CuboidSet& cuboids)
{
	MapWithCuboidKeys<PointFeature> output;
	for(const Cuboid& cuboid : cuboids)
		// TODO: RTreeData::queryGetAndRemoveAllWithCuboids.
		for(const auto& [subCuboid, pointFeature] : m_features.queryGetAllWithCuboids(cuboid))
			output.insertOrMerge(subCuboid, pointFeature);
	m_features.maybeRemove(cuboids);
	return output;
}
void Space::pointFeature_lock(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	const auto action = [&](PointFeature& feature){ feature.setLocked(true); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));

}
void Space::pointFeature_unlock(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	const auto action = [&](PointFeature& feature){ feature.setLocked(false); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
}
void Space::pointFeature_close(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	bool isTransparent;
	auto action = [&isTransparent](PointFeature& feature) mutable { isTransparent = MaterialType::getTransparent(feature.materialType); feature.setClosed(true); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_exteriorPortals.onCuboidCanNotTransmitTemperature(m_area, {point, point});
	if(!isTransparent)
	{
		if(pointFeatureType == PointFeatureTypeId::Hatch)
			m_area.m_visionCuboids.pointFloorIsOpaque(point);
		else
			m_area.m_visionCuboids.pointIsOpaque(point);
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	}
}
void Space::pointFeature_open(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == pointFeatureType; };
	bool isTransparent;
	auto action = [&isTransparent](PointFeature& feature) mutable { isTransparent = MaterialType::getTransparent(feature.materialType); feature.setClosed(false); };
	m_features.updateActionWithConditionOne(point, action, condition);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	if(!isTransparent)
	{
		if(pointFeatureType == PointFeatureTypeId::Hatch)
			m_area.m_visionCuboids.pointFloorIsTransparent(point);
		else
			m_area.m_visionCuboids.pointIsTransparent(point);
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	}
}
bool Space::pointFeature_canStandIn(const Point3D& point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).canStandIn; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_canStandAbove(const Point3D& point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).canStandAbove; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_isSupport(const Point3D& point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).isSupportAgainstCaveIn; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_canEnterFromBelow(const Point3D& point) const
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
bool Space::pointFeature_canEnterFromAbove([[maybe_unused]] const Point3D& point, const Point3D& from) const
{
	assert(shape_anythingCanEnterEver(point));
	return pointFeature_canEnterFromBelow(from);
}
MaterialTypeId Space::pointFeature_getMaterialType(const Point3D& point, const PointFeatureTypeId& featureType) const
{
	const auto condition = [&](const PointFeature& feature){ return feature.pointFeatureType == featureType; };
	const PointFeature& feature = m_features.queryGetOneWithCondition(point, condition);
	return feature.materialType;
}
MaterialTypeId Space::pointFeature_getMaterialTypeFirst(const Point3D& point) const
{
	const PointFeature& feature = m_features.queryGetFirst(point);
	if(feature.exists())
		return feature.materialType;
	else
		return MaterialTypeId::null();
}
bool Space::pointFeature_multiTileCanEnterAtNonZeroZOffset(const Point3D& point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).blocksMultiTileShapesIfNotAtZeroZOffset; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_isOpaque(const Point3D& point) const
{
	const auto condition = [&](const PointFeature& feature) { return PointFeatureType::byId(feature.pointFeatureType).opaque; };
	return m_features.queryAnyWithCondition(point, condition);
}
bool Space::pointFeature_floorIsOpaque(const Point3D& point) const
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