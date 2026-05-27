#include "space.h"
#include "../area/area.h"
bool Space::designation_anyForFaction(const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).any(designation);
}
bool Space::designation_has(const Point3D shape, const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).check(shape, designation);
}
bool Space::designation_has(const Cuboid shape, const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).check(shape, designation);
}
bool Space::designation_has(const CuboidSet& shape, const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).check(shape, designation);
}
Point3D Space::designation_hasPoint(const Cuboid shape, const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).queryPoint(shape, designation);
}
Point3D Space::designation_hasPoint(const CuboidSet& shape, const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).queryPoint(shape, designation);
}
CuboidSet Space::designation_queryForFaction(const Cuboid cuboid, const FactionId faction, const SpaceDesignation designation) const
{
	return m_area.m_spaceDesignations.getForFaction(faction).queryGetIntersection(cuboid, designation);
}
void Space::designation_set(const Point3D shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).set(shape, designation);
}
void Space::designation_set(const Cuboid shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).set(shape, designation);
}
void Space::designation_set(const CuboidSet& shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).set(shape, designation);
}
void Space::designation_unset(const Point3D shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).unset(shape, designation);
}
void Space::designation_unset(const Cuboid shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).unset(shape, designation);
}
void Space::designation_unset(const CuboidSet& shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).unset(shape, designation);
}
void Space::designation_maybeUnset(const Point3D shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(shape, designation);
}
void Space::designation_maybeUnset(const Cuboid shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(shape, designation);
}
void Space::designation_maybeUnset(const CuboidSet& shape, const FactionId faction, const SpaceDesignation designation)
{
	m_area.m_spaceDesignations.getForFaction(faction).maybeUnset(shape, designation);
}