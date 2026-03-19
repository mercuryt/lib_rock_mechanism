#include "exposedToSky.h"
#include "../space/space.h"
#include "../area/area.h"
#include "../area/exteriorPortals.h"
void PointsExposedToSky::initialize(const Cuboid cuboid)
{
	m_data.maybeInsert(cuboid);
}
void PointsExposedToSky::maybeSetCuboid(Area& area, const Cuboid cuboid)
{
	/*
		Set true for the cuboid as well as everything under it that is not below a solid layer or a floor or hatch.
		Includes the topmost solid layer.
		This should be run after the cuboid is removed from Space::m_solid.
	*/
	// Get exposed blocks on the top face of the cuboid.
	Cuboid above = cuboid.getFaceAbove();
	CuboidSet exposed;
	if(cuboid.m_high.z() == area.getSpace().m_sizeZ - 1)
		exposed = CuboidSet::create(above);
	else
		exposed = m_data.queryGetLeaves(above).intersection(above);
	// Extend exposed to the bottom of the space.
	for(Cuboid& exposedCuboid : exposed)
		exposedCuboid.m_low.setZ({0});
	Space& space = area.getSpace();
	// Remove solid and below from exposed.
	space.solid_queryForEachCuboid(exposed.boundry(), [&](const Cuboid solidCuboid){
		Cuboid cuboidShadow = solidCuboid;
		cuboidShadow.m_low.setZ({0});
		exposed.maybeRemove(cuboidShadow);
	});
	// Remove features and below.
	space.pointFeature_queryForEachWithCuboids(exposed.boundry(), [&](const Cuboid featureCuboid, const PointFeature feature){
		if(feature.pointFeatureType != PointFeatureTypeId::FloorGrate )
		{
			Cuboid shadow = featureCuboid;
			shadow.m_low.setZ({0});
			exposed.maybeRemove(shadow);
		}
	});
	// Include the top layer of solid ground or floor.
	for(Cuboid& exposedCuboid : exposed)
		if(exposedCuboid.m_high.z() != 0)
			exposedCuboid.m_high.setZ(cuboid.m_high.z() - 1);
	m_data.maybeInsert(exposed);
	// record meltable.
	space.solid_queryForEachWithCuboids(exposed, [&](const Cuboid materialCuboid, const MaterialTypeId materialType){
		const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
		if(meltingPoint.exists())
			area.m_hasTemperature.maybeAddMeltableSolidPointsAboveGround(exposed.intersection(materialCuboid), meltingPoint);
	});
	space.plant_updateGrowingStatus(exposed);
	// Check for overhangs to mark as portals.
	CuboidSet adjacentToExposed = exposed.getAdjacent();
	adjacentToExposed = space.temperature_queryTransmitsCuboidsIntersection(adjacentToExposed);
	area.m_exteriorPortals.maybeAddAll(area, adjacentToExposed);
}
void PointsExposedToSky::set(Area& area, const Point3D point)
{
	return maybeSetCuboid(area, Cuboid::create(point, point));
	Point3D current = point;
	Space& space = area.getSpace();
	while(true)
	{
		m_data.maybeInsert(current);
		if(space.solid_isAny(current))
		{
			const MaterialTypeId materialType = space.solid_get(current);
			const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
			if(meltingPoint.exists())
				area.m_hasTemperature.maybeAddMeltableSolidPointsAboveGround(CuboidSet::create(current), meltingPoint);
			break;
		}
		// Check for adjacent which are now portals because they are adjacent to a point exposed to sky.
		for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(current))
			if(
				space.temperature_transmits(adjacent) &&
				AreaHasExteriorPortals::isPortal(space, adjacent) &&
				!area.m_exteriorPortals.isRecordedAsPortal(adjacent)
			)
				area.m_exteriorPortals.add(area, adjacent);
		// Check if this point is no longer a portal.
		if(area.m_exteriorPortals.isRecordedAsPortal(current))
			area.m_exteriorPortals.remove(area, current);
		space.plant_updateGrowingStatus(current);
		if(current.z() == 0)
			break;
		current = current.below();
	}
}
void PointsExposedToSky::unset(Area& area, const Point3D point)
{
	return maybeUnset(area, Cuboid::create(point, point));
	Point3D current = point;
	Space& space = area.getSpace();
	while(check(current))
	{
		m_data.maybeRemove(current);
		// Check for adjacent which are no longer portals because they are no longer adjacent to a point exposed to sky.
		for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(current))
			if(area.m_exteriorPortals.isRecordedAsPortal(adjacent) && (!space.temperature_transmits(adjacent) || !AreaHasExteriorPortals::isPortal(space, adjacent)))
				area.m_exteriorPortals.remove(area, adjacent);
		if(space.solid_isAny(current))
		{
			const MaterialTypeId materialType = space.solid_get(current);
			const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
			if(meltingPoint.exists())
				area.m_hasTemperature.maybeRemoveMeltableSolidPointsAboveGround(Cuboid::create(current, current), meltingPoint);
			break;
		}
		// Check if this point is now a portal.
		if(space.temperature_transmits(current) && AreaHasExteriorPortals::isPortal(space, current))
			area.m_exteriorPortals.add(area, current);
		space.plant_updateGrowingStatus(current);
		if(current.z() == 0)
			break;
		current = current.below();
	}
}
void PointsExposedToSky::maybeUnset(Area& area, const Cuboid cuboid)
{
	Cuboid shadow = cuboid;
	shadow.m_low.setZ({0});
	Space& space = area.getSpace();
	CuboidSet setInShadow = m_data.queryGetLeaves(shadow).intersection(shadow);
	m_data.maybeRemove(shadow);
	CuboidSet adjacentToSetInShadow = setInShadow.getAdjacent();
	area.m_exteriorPortals.maybeRemoveAll(area, adjacentToSetInShadow);
	area.m_exteriorPortals.maybeAddAll(area, setInShadow);
	space.plant_updateGrowingStatus(setInShadow);
	space.solid_queryForEachWithCuboids(setInShadow, [&](const Cuboid solidCuboid, const MaterialTypeId materialType){
			const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
			if(meltingPoint.exists())
				area.m_hasTemperature.maybeRemoveMeltableSolidPointsAboveGround(setInShadow.intersection(solidCuboid), meltingPoint);
	});
}
bool PointsExposedToSky::check(const CuboidSet& cuboids) const { return m_data.query(cuboids); }
bool PointsExposedToSky::check(const Cuboid cuboid) const { return m_data.query(cuboid); }
bool PointsExposedToSky::check(const Point3D point) const { return m_data.query(point); }