#include "exposedToSky.h"
#include "../space/space.h"
#include "../area/area.h"
#include "../area/exteriorPortals.h"
void PointsExposedToSky::initialize(const Cuboid& cuboid)
{
	m_data.maybeInsert(cuboid);
}
void PointsExposedToSky::set(Area& area, const Point3D& point)
{
	Point3D current = point;
	Space& space = area.getSpace();
	while(true)
	{
		m_data.maybeInsert(current);
		if(space.solid_isAny(current))
		{
			const MaterialTypeId& materialType = space.solid_get(current);
			if(MaterialType::canMelt(materialType))
				area.m_hasTemperature.maybeAddMeltableSolidPointAboveGround(current);
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
void PointsExposedToSky::unset(Area& area, const Point3D& point)
{
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
			const MaterialTypeId& materialType = space.solid_get(current);
			if(MaterialType::canMelt(materialType))
				area.m_hasTemperature.maybeRemoveMeltableSolidPointAboveGround(current);
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