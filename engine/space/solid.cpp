#include "space.h"
#include "../area/area.h"
#include "../definitions/materialType.h"
#include "../definitions/plantSpecies.h"
#include "../numericTypes/types.h"
#include "../pointFeature.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../portables.h"

void Space::solid_setShared(const Point3D& point, const MaterialTypeId& materialType, bool wasEmpty)
{
	assert(!m_items.queryAny(point));
	if(wasEmpty)
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	fluid_onPointSetSolid(point);
	// Space below are no longer exposed to sky.
	if(point.z() != 0)
	{
		const Point3D& below = point.below();
		if(m_exposedToSky.check(below))
			m_exposedToSky.unset(m_area, below);
	}
	// Record as meltable.
	if(m_exposedToSky.check(point) && MaterialType::canMelt(materialType))
		m_area.m_hasTemperature.maybeAddMeltableSolidPointAboveGround(point);
	// Remove from stockpiles.
	m_area.m_hasStockPiles.removePointFromAllFactions(point);
}
void Space::solid_set(const Point3D& point, const MaterialTypeId& materialType, bool constructed)
{
	solid_setCuboid({point, point}, materialType, constructed);
}
void Space::solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed)
{
	// TODO: This assumes previous is all one type.
	const MaterialTypeId& previous = m_solid.queryGetOne(cuboid);
	bool wasTransparent = (previous.empty() || MaterialType::getTransparent(previous));
	m_solid.maybeInsertOrOverwrite(cuboid, materialType);
	if(constructed)
		m_constructed.maybeInsert(cuboid);
	// TODO: why?
	m_visible.maybeInsert(cuboid);
	Plants& plants = m_area.getPlants();
	const auto& plantIndices = m_plants.queryGetAll(cuboid);
	for(const PlantIndex& plant : plantIndices)
	{
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant)));
		plants.die(plant);
	}
	m_area.m_hasTerrainFacades.maybeSetImpassable(cuboid);
	m_area.m_exteriorPortals.onCuboidCanNotTransmitTemperature(m_area, cuboid);
	m_area.m_hasCraftingLocationsAndJobs.maybeRemoveCuboid(cuboid);
	for(const Point3D& point : cuboid)
		solid_setShared(point, materialType, previous.empty());
	m_support.set(cuboid);
	// Vision cuboid.
	if(!MaterialType::getTransparent(materialType) && wasTransparent)
	{
		m_area.m_visionCuboids.cuboidIsOpaque(cuboid);
		m_area.m_opacityFacade.maybeInsertFull(cuboid);
	}
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid point.
	m_reservables.maybeRemove(cuboid);
}
void Space::solid_setNotCuboid(const Cuboid& cuboid)
{
	const auto& previous = m_solid.queryGetAll(cuboid);
	if(previous.empty())
		return;
	bool wasTransparent = true;
	for(const MaterialTypeId& material : previous)
		if(!MaterialType::getTransparent(material))
		{
			wasTransparent = false;
			break;
		}
	m_constructed.maybeRemove(cuboid);
	// TODO: Why are we doing this? What does visible actually mean?
	m_visible.maybeInsert(cuboid);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid point.
	m_reservables.maybeRemove(cuboid);
	const Cuboid spaceBoundry = boundry();
	for(const Point3D& point : cuboid)
	{
		if(!solid_isAny(point))
			continue;
		if(m_exposedToSky.check(point) && MaterialType::canMelt(m_solid.queryGetOne(point)))
			m_area.m_hasTemperature.maybeRemoveMeltableSolidPointAboveGround(point);
		fluid_onPointSetNotSolid(point);
		setBelowVisible(point);
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(point);
	}
	m_solid.maybeRemove(cuboid);
	m_support.unset(cuboid);
	Cuboid inflated = cuboid;
	inflated.inflate({1});
	m_area.m_hasTerrainFacades.update(spaceBoundry.intersection(inflated));
	for(const Point3D& point : cuboid)
	{
		m_area.m_exteriorPortals.onPointCanTransmitTemperature(m_area, point);
		if(point.z() != 0 && m_exposedToSky.check(point))
		{
			const Point3D& below = point.below();
			if(spaceBoundry.contains(below) && !m_exposedToSky.check(below))
				m_exposedToSky.set(m_area, below);
		}
	}
	// Vision cuboid.
	if(!wasTransparent)
	{
		m_area.m_visionCuboids.cuboidIsTransparent(cuboid);
		m_area.m_opacityFacade.maybeRemoveFull(cuboid);
	}
	// Gravity.
	const Point3D& aboveHighest = cuboid.m_high.above();
	if(aboveHighest.exists())
	{
		Cuboid aboveCuboid = cuboid.getFace(Facing6::Above);
		aboveCuboid.shift(Facing6::Above, Distance::create(1));
		for(const Point3D& above : aboveCuboid)
			maybeContentsFalls(above);
	}
}
MapWithCuboidKeys<MaterialTypeId> Space::solid_getAllWithCuboidsAndRemove(const CuboidSet& cuboids)
{
	MapWithCuboidKeys<MaterialTypeId> output;
	for(const Cuboid& cuboid : cuboids)
		for(const auto& [materialCuboid, materialType] : m_solid.queryGetAllWithCuboids(cuboid))
			output.insertOrMerge(materialCuboid, materialType);
	m_solid.maybeRemove(cuboids);
	return output;
}
Mass Space::solid_getMass(const Point3D& point) const
{
	const MaterialTypeId& materialType = m_solid.queryGetOne(point);
	assert(!materialType.empty());
	return Config::maxPointVolume.toVolume() * MaterialType::getDensity(materialType);
}
Mass Space::solid_getMass(const CuboidSet& cuboidSet) const
{
	// TODO: use m_solid.queryCount(cuboidSet).
	Mass output;
	for(const Cuboid& cuboid : cuboidSet)
		for(const Point3D& point : cuboid)
			output += solid_getMass(point);
	return output;
}
std::pair<MaterialTypeId, int32_t> Space::solid_getHardest(const CuboidSet& cuboids)
{
	// Check Solid first.
	const auto predicateSolid = [&](const MaterialTypeId& materialType) { return MaterialType::getHardness(materialType); };
	MaterialTypeId output;
	int32_t hardness = 0;
	for(const Cuboid& cuboid : cuboids)
	{
		const auto [materialType, cuboidHardness] = m_solid.queryGetHighestReturnWithPredicateOutput<int32_t, predicateSolid>(cuboid);
		if(cuboidHardness > hardness)
		{
			hardness = cuboidHardness;
			output = materialType;
		}
	}
	if(output.exists())
		return {output, hardness};
	// No solid found, check features.
	const auto predicateFeature = [&](const PointFeature& feature) { return MaterialType::getHardness(feature.materialType); };
	for(const Cuboid& cuboid : cuboids)
	{
		const auto [feature, cuboidHardness] = m_features.queryGetHighestReturnWithPredicateOutput<int32_t, predicateFeature>(cuboid);
		if(cuboidHardness > hardness)
		{
			hardness = cuboidHardness;
			output = feature.materialType;
		}
	}
	return {output, hardness};
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
void Space::solid_setCuboidDynamic(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed)
{
	assert(!m_dynamic.query(cuboid));
	assert(!m_plants.queryAny(cuboid));
	assert(!m_items.queryAny(cuboid));
	assert(!m_actors.queryAny(cuboid));
	m_solid.maybeInsert(cuboid, materialType);
	m_dynamic.maybeInsert(cuboid);
	if(constructed)
		m_constructed.maybeInsert(cuboid);
	if(!MaterialType::getTransparent(materialType))
	{
		m_area.m_opacityFacade.update(m_area, cuboid);
		// TODO: add a multiple space at a time version which does this update more efficiently.
		m_area.m_visionCuboids.cuboidIsOpaque(cuboid);
		// Opacity.
		if(!MaterialType::getTransparent(materialType))
			m_area.m_visionCuboids.cuboidIsOpaque(cuboid);
	}
}
void Space::solid_setNotDynamic(const Point3D& point)
{
	assert(m_dynamic.query(point));
	const MaterialTypeId material = m_solid.queryGetOne(point);
	bool wasTransparent = material.empty() ? false : MaterialType::getTransparent(material);
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