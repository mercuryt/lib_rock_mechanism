#include "space.h"
#include "../area/area.h"
#include "../pointFeature.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../reference.h"
#include "../numericTypes/types.h"
bool Space::pointFeature_contains(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const
{
	return m_features.queryGetOne(point).contains(pointFeatureType);
}
const PointFeatureSet& Space::pointFeature_getAll(const Point3D& point) const { return m_features.queryGetOne(point); }
// Can return nullptr.
const PointFeature* Space::pointFeature_at(const Point3D& point, const PointFeatureTypeId& pointFeatureType) const
{
	return m_features.queryGetOne(point).maybeGet(pointFeatureType);
}
void Space::pointFeature_remove(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(!solid_is(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	const bool wasOpaque = pointFeature_isOpaque(point);
	const bool floorWasOpaque = pointFeature_floorIsOpaque(point);
	const auto& features = m_features.queryGetOne(point);
	assert(!features.empty());
	if(features.size() == 1)
	{
		auto featuresCopy = features;
		featuresCopy.remove(pointFeatureType);
		m_features.updateOne(point, features, std::move(featuresCopy));
	}
	else
		m_features.remove(point);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_hasTerrainFacades.update({point, point});
	if(!transmitedTemperaturePreviously && temperature_transmits(point))
		m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	if(wasOpaque && !pointFeature_isOpaque(point))
		m_area.m_visionCuboids.pointIsTransparent(point);
	if(floorWasOpaque && !pointFeature_floorIsOpaque(point))
		m_area.m_visionCuboids.pointFloorIsTransparent(point);
}
void Space::pointFeature_removeAll(const Point3D& point)
{
	assert(!solid_is(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	const bool wasOpaque = pointFeature_isOpaque(point);
	const bool floorWasOpaque = pointFeature_floorIsOpaque(point);
	m_features.remove(point);
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_hasTerrainFacades.update({point, point});
	if(!transmitedTemperaturePreviously && temperature_transmits(point))
		m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	if(wasOpaque)
		m_area.m_visionCuboids.pointIsTransparent(point);
	if(floorWasOpaque)
		m_area.m_visionCuboids.pointFloorIsTransparent(point);
}
void Space::pointFeature_construct(const Point3D& point, const PointFeatureTypeId& pointFeatureType, const MaterialTypeId& materialType)
{
	assert(!solid_is(point));
	const bool transmitedTemperaturePreviously = temperature_transmits(point);
	const bool materialTypeIsTransparent = MaterialType::getTransparent(materialType);
	Plants& plants = m_area.getPlants();
	if(plant_exists(point))
	{
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant_get(point))));
		plant_erase(point);
	}
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	const auto& features = m_features.queryGetOne(point);
	auto featuresCopy = features;
	// Hewn, closed, locked.
	featuresCopy.insert(pointFeatureType, materialType, false, true, false);
	m_features.insertOrOverwrite(point, std::move(featuresCopy));
	const bool isFloorOrHatch = (pointFeatureType == PointFeatureTypeId::Floor || pointFeatureType == PointFeatureTypeId::Hatch);
	if(isFloorOrHatch && !MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.pointFloorIsOpaque(point);
		m_exposedToSky.unset(m_area, point);
	}
	else if(PointFeatureType::byId(pointFeatureType).pointIsOpaque && !materialTypeIsTransparent)
	{
		m_area.m_visionCuboids.pointIsOpaque(point);
		m_exposedToSky.unset(m_area, point);
	}
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
	if(transmitedTemperaturePreviously && !temperature_transmits(point))
		m_area.m_exteriorPortals.onPointCanNotTransmitTemperature(m_area, point);
}
void Space::pointFeature_hew(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(solid_is(point));
	const MaterialTypeId& materialType = solid_get(point);
	const bool materialTypeIsTransparent = MaterialType::getTransparent(materialType);
	const auto& features = m_features.queryGetOne(point);
	auto featuresCopy = features;
	// Hewn, closed, locked.
	featuresCopy.insert(pointFeatureType, materialType, true, true, false);
	m_features.updateOne(point, features, std::move(featuresCopy));
	// Solid_setNot will handle calling m_exteriorPortals.onPointCanTransmitTemperature.
	// TODO: There is no support for hewing hatches or flaps. This is ok because those things can't be hewn. Could be fixed anyway?
	const Point3D& above = point.above();
	auto actorsCopy = actor_getAll(above);
	for(const ActorIndex& actor : actorsCopy)
		m_area.getActors().tryToMoveSoAsNotOccuping(actor, above);
	solid_setNot(point);
	m_area.m_opacityFacade.update(m_area, point);
	if(!PointFeatureType::byId(pointFeatureType).pointIsOpaque && !materialTypeIsTransparent)
		// Neighter floor nor hatch can be hewn so we don't need to check if this is point opaque or floor opaque.
		m_area.m_visionCuboids.pointIsOpaque(point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_hasTerrainFacades.update(getAdjacentWithEdgeAndCornerAdjacent(point));
}
void Space::pointFeature_setTemperature(const Point3D& point, const Temperature& temperature)
{
	for(const PointFeature& feature : m_features.queryGetOne(point))
	{
		if(MaterialType::getIgnitionTemperature(feature.materialType).exists() &&
			temperature > MaterialType::getIgnitionTemperature(feature.materialType)
		)
			m_area.m_fires.ignite(point, feature.materialType);
	}
}
void Space::pointFeature_setAll(const Point3D& point, PointFeatureSet& features)
{
	assert(m_features.queryGetOne(point).empty());
	m_features.insert(point, std::move(features));
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_hasTerrainFacades.update({point, point});
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	if(pointFeature_isOpaque(point))
		m_area.m_visionCuboids.pointIsOpaque(point);
	if(pointFeature_floorIsOpaque(point))
		m_area.m_visionCuboids.pointFloorIsOpaque(point);
}
void Space::pointFeature_lock(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto& features = m_features.queryGetOne(point);
	auto featuresCopy = features;
	auto& feature = featuresCopy.get(pointFeatureType);
	feature.locked = true;
	m_features.updateOne(point, features, std::move(featuresCopy));
	m_area.m_hasTerrainFacades.update({point, point});
}
void Space::pointFeature_unlock(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto& features = m_features.queryGetOne(point);
	auto featuresCopy = features;
	auto& feature = featuresCopy.get(pointFeatureType);
	feature.locked = false;
	m_features.updateOne(point, features, std::move(featuresCopy));
	m_area.m_hasTerrainFacades.update({point, point});
}
void Space::pointFeature_close(const Point3D& point, const PointFeatureTypeId& pointFeatureType)
{
	assert(pointFeature_contains(point, pointFeatureType));
	const auto& features = m_features.queryGetOne(point);
	auto featuresCopy = features;
	auto& feature = featuresCopy.get(pointFeatureType);
	assert(!feature.closed);
	feature.closed = true;
	m_features.updateOne(point, features, std::move(featuresCopy));
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_exteriorPortals.onPointCanNotTransmitTemperature(m_area, point);
	if(!MaterialType::getTransparent(feature.materialType))
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
	const auto& features = m_features.queryGetOne(point);
	auto featuresCopy = features;
	auto& feature = featuresCopy.get(pointFeatureType);
	assert(!feature.closed);
	feature.closed = true;
	m_features.updateOne(point, features, std::move(featuresCopy));
	m_area.m_opacityFacade.update(m_area, point);
	m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	if(!MaterialType::getTransparent(feature.materialType))
	{
		if(pointFeatureType == PointFeatureTypeId::Hatch)
			m_area.m_visionCuboids.pointFloorIsTransparent(point);
		else
			m_area.m_visionCuboids.pointIsTransparent(point);
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	}
}
// Space entrance from all angles, does not include floor and hatch which only point from below.
bool Space::pointFeature_blocksEntrance(const Point3D& point) const
{
	for(const PointFeature& pointFeature : m_features.queryGetOne(point))
	{
		if(
			pointFeature.pointFeatureTypeId == PointFeatureTypeId::Fortification ||
			pointFeature.pointFeatureTypeId == PointFeatureTypeId::FloodGate ||
			(pointFeature.pointFeatureTypeId == PointFeatureTypeId::Door && pointFeature.locked)
		 )
			return true;
	}
	return false;
}
bool Space::pointFeature_canStandIn(const Point3D& point) const
{
	return m_features.queryGetOne(point).canStandIn();
}
bool Space::pointFeature_canStandAbove(const Point3D& point) const
{
	for(const PointFeature& pointFeature : m_features.queryGetOne(point))
		if(PointFeatureType::byId(pointFeature.pointFeatureTypeId).canStandAbove)
			return true;
	return false;
}
bool Space::pointFeature_isSupport(const Point3D& point) const
{
	for(const PointFeature& pointFeature : m_features.queryGetOne(point))
		if(PointFeatureType::byId(pointFeature.pointFeatureTypeId).isSupportAgainstCaveIn)
			return true;
	return false;
}
bool Space::pointFeature_canEnterFromBelow(const Point3D& point) const
{
	for(const PointFeature& pointFeature : m_features.queryGetOne(point))
		if(
			pointFeature.pointFeatureTypeId == PointFeatureTypeId::Floor ||
			pointFeature.pointFeatureTypeId == PointFeatureTypeId::FloorGrate ||
			(pointFeature.pointFeatureTypeId == PointFeatureTypeId::Hatch && pointFeature.locked)
		 )
			return false;
	return true;
}
bool Space::pointFeature_canEnterFromAbove([[maybe_unused]] const Point3D& point, const Point3D& from) const
{
	assert(shape_anythingCanEnterEver(point));
	for(const PointFeature& pointFeature : m_features.queryGetOne(from))
		if(
			pointFeature.pointFeatureTypeId == PointFeatureTypeId::Floor ||
			pointFeature.pointFeatureTypeId == PointFeatureTypeId::FloorGrate ||
			(pointFeature.pointFeatureTypeId == PointFeatureTypeId::Hatch && pointFeature.locked)
		 )
			return false;
	return true;
}
MaterialTypeId Space::pointFeature_getMaterialType(const Point3D& point) const
{
	if(m_features.queryGetOne(point).empty())
		return MaterialTypeId::null();
	return m_features.queryGetOne(point).front().materialType;
}
bool Space::pointFeature_empty(const Point3D& point) const
{
	return m_features.queryGetOne(point).empty();
}
bool Space::pointFeature_multiTileCanEnterAtNonZeroZOffset(const Point3D& point) const
{
	for(const PointFeature& pointFeature : m_features.queryGetOne(point))
		if(PointFeatureType::byId(pointFeature.pointFeatureTypeId).blocksMultiTileShapesIfNotAtZeroZOffset)
			return false;
	return true;
}
bool Space::pointFeature_isOpaque(const Point3D& point) const
{
	return m_features.queryGetOne(point).isOpaque();
}
bool Space::pointFeature_floorIsOpaque(const Point3D& point) const
{
	return m_features.queryGetOne(point).floorIsOpaque();
}