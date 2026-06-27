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

void Space::solid_set(const Point3D point, const MaterialTypeId materialType, bool constructed)
{
	solid_setCuboid({point, point}, materialType, constructed);
}
void Space::solid_setAll(const CuboidSet& cuboids, const MaterialTypeId materialType, bool constructed)
{
	assert(!cuboids.empty());
	for(const Cuboid cuboid : cuboids)
		solid_setCuboid(cuboid, materialType, constructed);
}
void Space::solid_setCuboid(const Cuboid cuboid, const MaterialTypeId materialType, bool constructed)
{
	assert(!m_items.queryAny(cuboid));
	// TODO: This assumes previous is all one type.
	const MaterialTypeId previous = m_solid.queryGetOne(cuboid);
	bool wasTransparent = (previous.empty() || MaterialType::getTransparent(previous));
	m_solid.maybeInsertOrOverwrite(cuboid, materialType);
	if(constructed)
		m_constructed.maybeInsert(cuboid);
	// Query all visible space adjacent to cuboid, inflate and then remove from toSetUnrevealed.
	// To get all visible space we must query all opaque space and invert it.
	// The result is all space within cuboid which is not adjacent to visible space.
	CuboidSet toSetUnrevealed = CuboidSet::create(cuboid);
	CuboidSet visibleSpace = toSetUnrevealed;
	Cuboid inflated = cuboid;
	inflated.inflate({1});
	m_area.m_opacityFacade.removeFromCuboidSet(visibleSpace);
	for(Cuboid visibleCuboid : visibleSpace)
	{
		visibleCuboid.inflate({1});
		toSetUnrevealed.maybeRemove(visibleCuboid);
	}
	m_unrevealed.maybeInsert(toSetUnrevealed);
	// Kill any plants.
	Plants& plants = m_area.getPlants();
	// TODO: should this be kill all or destroy all?
	plants.destroyAll(cuboid);
	m_area.m_hasPaths.maybeSetImpassable(cuboid);
	m_area.m_hasTemperature.onTemperatureCanNoLongerTransmit(m_area, CuboidSet::create(cuboid));
	m_area.m_hasCraftingLocationsAndJobs.maybeRemoveCuboid(cuboid);
	if(previous.empty())
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(cuboid);
	m_area.m_hasTemperature.onSetSolid(m_area, CuboidSet::create(cuboid), materialType);
	// Remove from stockpiles.
	m_area.m_hasStockPiles.removeFromAll(cuboid);
	// Remove fluids and shift them elsewhere.
	fluid_onSetSolid(cuboid.toSet());
	m_exposedToSky.maybeUnsetBeneathTopLayer(m_area, cuboid);
	m_support.set(cuboid);
	// Vision cuboid.
	if(!MaterialType::getTransparent(materialType) && wasTransparent)
		m_area.m_opacityFacade.maybeInsertFull(cuboid);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid point.
	m_reservables.maybeRemove(cuboid);
}
void Space::solid_setNotAll(const CuboidSet& cuboidSet)
{
	for(const Cuboid cuboid : cuboidSet)
		solid_setNotCuboid(cuboid);
}
void Space::solid_setNotCuboid(const Cuboid cuboid)
{
	const auto& previous = m_solid.queryGetAll(cuboid);
	if(previous.empty())
		return;
	bool wasTransparent = true;
	for(const MaterialTypeId material : previous)
		if(!MaterialType::getTransparent(material))
		{
			wasTransparent = false;
			break;
		}
	m_constructed.maybeRemove(cuboid);
	// TODO: Why are we doing this? What does visible actually mean?
	m_unrevealed.maybeRemove(cuboid);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid point.
	m_reservables.maybeRemove(cuboid);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(cuboid);
	const MaterialTypeId materialType = m_solid.queryGetOne(cuboid);
	m_solid.maybeRemove(cuboid);
	m_support.unset(cuboid);
	Cuboid inflated = cuboid;
	inflated.inflate({1});
	Cuboid spaceBoundry = boundry();
	m_area.m_hasPaths.update(m_area, spaceBoundry.intersection(inflated));
	m_exposedToSky.maybeSetCuboid(m_area, cuboid);
	m_area.m_hasTemperature.onSetNotSolid(m_area, CuboidSet::create(cuboid), materialType);
	fluid_onSetNotSolid(cuboid.toSet());
	// Vision cuboid.
	if(!wasTransparent)
		m_area.m_opacityFacade.maybeRemoveFull(cuboid);
	// Gravity.
	if(cuboid.m_high.z() != m_sizeZ - 1)
	{
		Cuboid aboveCuboid = cuboid.getFace(Facing6::Above);
		aboveCuboid.shift(Facing6::Above, Distance::create(1));
		maybeContentsFalls(aboveCuboid);
	}
}
MapWithCuboidKeys<MaterialTypeId> Space::solid_getAllWithCuboidsAndRemove(const CuboidSet& cuboids)
{
	MapWithCuboidKeys<MaterialTypeId> output;
	for(const Cuboid cuboid : cuboids)
		for(const auto& [materialCuboid, materialType] : m_solid.queryGetAllWithCuboids(cuboid))
			output.insertOrMerge(materialCuboid, materialType);
	m_solid.maybeRemove(cuboids);
	return output;
}
Mass Space::solid_getMass(const Point3D point) const
{
	const MaterialTypeId materialType = m_solid.queryGetOne(point);
	assert(!materialType.empty());
	return Config::maxPointVolume.toVolume() * MaterialType::getDensity(materialType);
}
Mass Space::solid_getMass(const CuboidSet& cuboidSet) const
{
	// TODO: use m_solid.queryCount(cuboidSet).
	Mass output;
	for(const Cuboid cuboid : cuboidSet)
		for(const Point3D point : cuboid)
			output += solid_getMass(point);
	return output;
}
std::pair<MaterialTypeId, int> Space::solid_getHardest(const CuboidSet& cuboids)
{
	// Check Solid first.
	const auto predicateSolid = [&](const MaterialTypeId materialType) { return MaterialType::getHardness(materialType); };
	MaterialTypeId output;
	int hardness = 0;
	for(const Cuboid cuboid : cuboids)
	{
		const auto [materialType, cuboidHardness] = m_solid.queryGetHighestReturnWithPredicateOutput<int, predicateSolid>(cuboid);
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
	for(const Cuboid cuboid : cuboids)
	{
		const auto [feature, cuboidHardness] = m_features.queryGetHighestReturnWithPredicateOutput<int, predicateFeature>(cuboid);
		if(cuboidHardness > hardness)
		{
			hardness = cuboidHardness;
			output = feature.materialType;
		}
	}
	return {output, hardness};
}
void Space::solid_setDynamic(const Point3D point, const MaterialTypeId materialType, bool constructed)
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
		m_area.m_opacityFacade.update(m_area, point);
}
void Space::solid_setCuboidDynamic(const Cuboid cuboid, const MaterialTypeId materialType, bool constructed)
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
		m_area.m_opacityFacade.update(m_area, cuboid);
}
template<typename ShapeT>
void Space::solid_setNotDynamicBody(const ShapeT shape)
{
	assert(m_dynamic.query(shape));
	m_solid.maybeRemove(shape);
	m_dynamic.maybeRemove(shape);
	m_constructed.maybeRemove(shape);
	m_area.m_opacityFacade.update(m_area, shape);
}
void Space::solid_setNotDynamic(const CuboidSet& cuboids) { solid_setNotDynamicBody<const CuboidSet&>(cuboids); }
void Space::solid_setNotDynamic(const Cuboid cuboid) { solid_setNotDynamicBody<const Cuboid>(cuboid); }
void Space::solid_removeOpaque(CuboidSet& cuboids) const
{
	m_solid.queryForEachWithCuboids(cuboids, [&](const Cuboid cuboid, const MaterialTypeId materialType){
		if(!MaterialType::getTransparent(materialType))
			cuboids.remove(cuboid);
	});
}
void Space::solid_removeAllFrom(CuboidSet& cuboids) const
{
	auto copy = cuboids;
	m_solid.queryForEachCuboid(copy, [&](const Cuboid cuboid) { cuboids.maybeRemove(cuboid); });
}