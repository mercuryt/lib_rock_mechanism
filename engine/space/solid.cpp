#include "space.h"
#include "../area/area.h"
#include "../definitions/materialType.h"
#include "../numericTypes/types.h"
#include "../pointFeature.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../portables.hpp"

void Space::solid_setShared(const Point3D& point, const MaterialTypeId& materialType, bool constructed)
{
	if(m_solid.queryGetOne(point) == materialType)
		return;
	assert(m_itemVolume.queryGetOne(point).empty());
	Plants& plants = m_area.getPlants();
	if(m_plants.queryGetOne(point).exists())
	{
		PlantIndex plant = m_plants.queryGetOne(point);
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant)));
		plants.die(plant);
	}
	if(materialType == m_solid.queryGetOne(point))
		return;
	bool wasEmpty = m_solid.queryGetOne(point).empty();
	if(wasEmpty)
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_solid.maybeInsert(point, materialType);
	if(constructed)
		m_constructed.maybeInsert(point);
	fluid_onPointSetSolid(point);
	// TODO: why?
	m_visible.maybeInsert(point);
	// Space below are no longer exposed to sky.
	const Point3D& below = point.below();
	if(below.exists() && m_exposedToSky.check(below))
		m_exposedToSky.unset(m_area, below);
	// Record as meltable.
	if(m_exposedToSky.check(point) && MaterialType::canMelt(materialType))
		m_area.m_hasTemperature.addMeltableSolidPointAboveGround(point);
	// Remove from stockpiles.
	m_area.m_hasStockPiles.removePointFromAllFactions(point);
	m_area.m_hasCraftingLocationsAndJobs.maybeRemoveLocation(point);
	if(wasEmpty && m_reservables.queryAny(point))
		// There are no reservations which can exist on both a solid and not solid point.
		dishonorAllReservations(point);
	m_area.m_hasTerrainFacades.updatePointAndAdjacent(point);
	m_area.m_exteriorPortals.onPointCanNotTransmitTemperature(m_area, point);
}
void Space::solid_setNotShared(const Point3D& point)
{
	if(!solid_is(point))
		return;
	if(m_exposedToSky.check(point) && MaterialType::canMelt(m_solid.queryGetOne(point)))
		m_area.m_hasTemperature.removeMeltableSolidPointAboveGround(point);
	m_solid.maybeRemove(point);
	m_constructed.maybeRemove(point);
	fluid_onPointSetNotSolid(point);
	//TODO: Check if any adjacent are visible first?
	// TODO: Why are we doing this? What does visible actually mean?
	m_visible.maybeInsert(point);
	setBelowVisible(point);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid point.
	m_reservables.remove(point);
	m_area.m_hasTerrainFacades.updatePointAndAdjacent(point);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
	const Point3D& below = point.below();
	if(m_exposedToSky.check(point))
	{
		if(below.exists() && !m_exposedToSky.check(below))
			m_exposedToSky.set(m_area, below);
	}
}
void Space::solid_set(const Point3D& point, const MaterialTypeId& materialType, bool constructed)
{
	bool wasEmpty = m_solid.queryGetOne(point).empty();
	solid_setShared(point, materialType, constructed);
	// Opacity.
	if(!MaterialType::getTransparent(materialType) && wasEmpty)
	{
		m_area.m_visionCuboids.pointIsOpaque(point);
		m_area.m_opacityFacade.update(m_area, point);
	}
}
void Space::solid_setNot(const Point3D& point)
{
	bool wasTransparent = MaterialType::getTransparent(solid_get(point));
	solid_setNotShared(point);
	// Vision cuboid.
	if(!wasTransparent)
	{
		m_area.m_visionCuboids.pointIsTransparent(point);
		m_area.m_opacityFacade.update(m_area, point);
	}
	// Gravity.
	const Point3D& above = point.above();
	if(above.exists() && shape_anythingCanEnterEver(above))
		maybeContentsFalls(above);
}
void Space::solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed)
{
	for(const Point3D& point : cuboid)
		solid_setShared(point, materialType, constructed);
	// Vision cuboid.
	if(!MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.cuboidIsOpaque(cuboid);
		m_area.m_opacityFacade.maybeInsertFull(cuboid);
	}
}
void Space::solid_setNotCuboid(const Cuboid& cuboid)
{
	for(const Point3D& point : cuboid)
		solid_setNotShared(point);
	// Vision cuboid.
	m_area.m_visionCuboids.cuboidIsTransparent(cuboid);
	m_area.m_opacityFacade.maybeRemoveFull(cuboid);
	// Gravity.
	const Point3D& aboveHighest = cuboid.m_highest.above();
	if(aboveHighest.exists())
	{
		Cuboid aboveCuboid = cuboid.getFace(Facing6::Above);
		aboveCuboid.shift(Facing6::Above, Distance::create(1));
		for(const Point3D& above : aboveCuboid)
			maybeContentsFalls(above);
	}
}
MaterialTypeId Space::solid_get(const Point3D& point) const
{
	return m_solid.queryGetOne(point);
}
bool Space::solid_is(const Point3D& point) const
{
	return m_solid.queryGetOne(point).exists();
}
Mass Space::solid_getMass(const Point3D& point) const
{
	assert(solid_is(point));
	return MaterialType::getDensity(m_solid.queryGetOne(point)) * FullDisplacement::create(Config::maxPointVolume.get());
}
MaterialTypeId Space::solid_getHardest(const SmallSet<Point3D>& space)
{
	MaterialTypeId output;
	uint32_t hardness = 0;
	for(const Point3D& point : space)
	{
		MaterialTypeId materialType = solid_get(point);
		if(materialType.empty())
			materialType = pointFeature_getMaterialType(point);
		if(materialType.empty())
			continue;
		auto pointHardness = MaterialType::getHardness(materialType);
		if(pointHardness > hardness)
		{
			hardness = pointHardness;
			output = materialType;
		}
	}
	assert(output.exists());
	return output;
}
void Space::solid_setDynamic(const Point3D& point, const MaterialTypeId& materialType, bool constructed)
{
	assert(!m_solid.queryAny(point));
	assert(!m_dynamic.query(point));
	assert(!m_plants.queryAny(point));
	assert(!m_items.queryAny(point));
	assert(!m_actors.queryAny(point));
	m_solid.maybeInsert(point, materialType);
	m_dynamic.maybeInsert(point);
	if(constructed)
		m_constructed.maybeInsert(point);
	if(!MaterialType::getTransparent(materialType))
	{
		m_area.m_opacityFacade.update(m_area, point);
		// TODO: add a multiple space at a time version which does this update more efficiently.
		m_area.m_visionCuboids.pointIsOpaque(point);
		// Opacity.
		if(!MaterialType::getTransparent(materialType))
			m_area.m_visionCuboids.pointIsOpaque(point);
	}
}
void Space::solid_setNotDynamic(const Point3D& point)
{
	assert(m_solid.queryAny(point));
	assert(m_dynamic.query(point));
	bool wasTransparent = MaterialType::getTransparent(m_solid.queryGetOne(point));
	m_solid.maybeRemove(point);
	m_dynamic.maybeRemove(point);
	m_constructed.maybeRemove(point);
	if(!wasTransparent)
	{
		m_area.m_opacityFacade.update(m_area, point);
		// TODO: add a multiple space at a time version which does this update more efficiently.
		m_area.m_visionCuboids.pointIsTransparent(point);
	}
}