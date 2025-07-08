#include "space.h"
#include "../area/area.h"
bool Space::designation_has(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).check(point, designation);
}
void Space::designation_set(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).set(point, designation);
}
void Space::designation_unset(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).unset(point, designation);
}
void Space::designation_maybeUnset(const Point3D& point, const FactionId& faction, const SpaceDesignation& designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(point, designation);
}
