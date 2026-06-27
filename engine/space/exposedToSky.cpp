#include "exposedToSky.h"
#include "../space/space.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area/area.h"
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
	CuboidSet exposed = m_data.queryGetLeaves(above).intersection(above);
	if(exposed.empty())
		return;
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
	if(exposed.empty())
		return;
	// Remove features and below.
	space.pointFeature_queryForEachWithCuboids(exposed.boundry(), [&](const Cuboid featureCuboid, const PointFeature feature){
		if(feature.pointFeatureType != PointFeatureTypeId::FloorGrate )
		{
			Cuboid shadow = featureCuboid;
			shadow.m_low.setZ({0});
			exposed.maybeRemove(shadow);
		}
	});
	if(exposed.empty())
		return;
	space.plant_updateGrowingStatus(exposed);
	// Include the top layer of solid ground or floor.
	for(Cuboid& exposedCuboid : exposed)
		if(exposedCuboid.m_low.z() != 0)
			exposedCuboid.m_low.setZ(exposedCuboid.m_low.z() - 1);
	m_data.maybeInsert(exposed);
	// record meltable.
	space.solid_queryForEachWithCuboids(exposed, [&](const Cuboid materialCuboid, const MaterialTypeId materialType){
		const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
		if(meltingPoint.exists())
			area.m_hasTemperature.onSetSolid(area, exposed.intersection(materialCuboid), materialType);
	});
	// record freezable.
	space.fluid_queryForEachWithCuboids(exposed, [&](const Cuboid fluidCuboid, const FluidData fluidData){
		const Temperature freezingPoint = FluidType::getFreezingPoint(fluidData.type);
		if(freezingPoint.exists())
			area.m_hasTemperature.onFluidEnters(area, CuboidSet::create(fluidCuboid), fluidData.type, fluidData.group);
	});
	// Check for overhangs to mark as portals.
	CuboidSet adjacentToExposed = exposed.getAdjacent();
	adjacentToExposed = space.temperature_queryTransmitsCuboidsIntersection(adjacentToExposed);
	area.m_hasTemperature.m_portals.maybeAdd(area, adjacentToExposed);
	area.m_hasTemperature.m_portals.maybeRemove(area, exposed);
	area.m_hasTemperature.markToUpdate(exposed);
	// Mark actors and items as being on surface.
	Actors& actors = area.getActors();
	space.actor_queryForEach(exposed, [&](const ActorIndex actor){
		actors.setOnSurface(actor, true);
	});
	Items& items = area.getItems();
	space.item_queryForEach(exposed, [&](const ItemIndex item){
		items.setOnSurface(item, true);
	});
}
void PointsExposedToSky::set(Area& area, const Point3D point)
{
	return maybeSetCuboid(area, Cuboid::create(point, point));
}
void PointsExposedToSky::unset(Area& area, const Point3D point)
{
	return maybeUnsetBeneathTopLayer(area, Cuboid::create(point, point));
}
void PointsExposedToSky::maybeUnsetBeneathTopLayer(Area& area, const Cuboid cuboid)
{
	// Nothing below to unset.
	if(cuboid.m_high.z() == 0)
		return;
	Cuboid shadow = cuboid;
	shadow.m_low.setZ({0});
	shadow.m_high.setZ(shadow.m_high.z() - 1);
	Space& space = area.getSpace();
	CuboidSet setInShadow = m_data.queryGetLeaves(shadow);
	if(setInShadow.empty())
		return;
	setInShadow = setInShadow.intersection(shadow);
	m_data.maybeRemove(shadow);
	CuboidSet adjacentToSetInShadow = setInShadow.getAdjacent();
	area.m_hasTemperature.m_portals.maybeRemove(area, adjacentToSetInShadow);
	area.m_hasTemperature.m_portals.maybeAdd(area, setInShadow);
	area.m_hasTemperature.markToUpdate(shadow);
	space.plant_updateGrowingStatus(setInShadow);
	space.solid_queryForEachWithCuboids(setInShadow, [&](const Cuboid solidCuboid, const MaterialTypeId materialType){
		const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
		if(meltingPoint.exists())
			area.m_hasTemperature.onSetNotSolid(area, setInShadow.intersection(solidCuboid), materialType);
	});
	// unmark actors and items as being on surface.
	Actors& actors = area.getActors();
	space.actor_queryForEach(shadow, [&](const ActorIndex actor){
		actors.setOnSurface(actor, false);
	});
	Items& items = area.getItems();
	space.item_queryForEach(shadow, [&](const ItemIndex item){
		items.setOnSurface(item, false);
	});
}
bool PointsExposedToSky::check(const CuboidSet& cuboids) const { return m_data.query(cuboids); }
bool PointsExposedToSky::check(const Cuboid cuboid) const { return m_data.query(cuboid); }
bool PointsExposedToSky::check(const Point3D point) const { return m_data.query(point); }