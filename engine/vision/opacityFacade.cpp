#include "opacityFacade.h"

#include "../area/area.h"
#include "../numericTypes/types.h"
#include "../space/space.h"
#include "../geometry/paramaterizedLine.h"

void OpacityFacade::update(const Area& area, const Cuboid& cuboid)
{
	const Space& space = area.getSpace();
	if(space.canSeeThrough(cuboid))
		m_fullOpacity.maybeRemove(cuboid);
	else
		m_fullOpacity.maybeInsert(cuboid);
	if(space.canSeeThroughFloor(cuboid))
		m_floorOpacity.maybeRemove(cuboid);
	else
		m_floorOpacity.maybeInsert(cuboid);
}
void OpacityFacade::maybeInsertFull(const Cuboid& cuboid) { m_fullOpacity.maybeInsert(cuboid); }
void OpacityFacade::maybeRemoveFull(const Cuboid& cuboid) { m_fullOpacity.maybeRemove(cuboid); }
bool OpacityFacade::hasLineOfSight(const Point3D& fromCoords, const Point3D& toCoords) const
{
	ParamaterizedLine line(fromCoords, toCoords);
	if(m_fullOpacity.query(line))
		return false;
	if(fromCoords.z() != toCoords.z())
		return !m_floorOpacity.query(line);
	return true;
}
std::vector<bool> OpacityFacade::hasLineOfSightBatched(const std::vector<std::pair<Point3D, Point3D>>& coords) const
{
	const std::vector<ParamaterizedLine> lines(coords.begin(), coords.end());
	std::vector<bool> blocked = m_fullOpacity.batchQuery(lines);
	std::vector<ParamaterizedLine> linesNotBlockedByFullOpacity;
	SmallMap<int, int> offsetInLinesByOffsetInLinesNotBlocked;
	const auto& begin = blocked.begin();
	const auto& end = blocked.end();
	for(auto iter = begin; iter != end; ++iter)
		if(!*iter)
		{
			int offset = std::distance(begin, iter);
			linesNotBlockedByFullOpacity.push_back(lines[offset]);
			offsetInLinesByOffsetInLinesNotBlocked.insert(linesNotBlockedByFullOpacity.size(), offset);
		}
	const std::vector<bool> blockedByFloor = m_floorOpacity.batchQuery(linesNotBlockedByFullOpacity);
	const auto& begin2 = blockedByFloor.begin();
	const auto& end2 = blockedByFloor.end();
	for(auto iter = begin2; iter != end2; ++iter)
		if(!*iter)
		{
			int offset = std::distance(begin2, iter);
			blocked[offsetInLinesByOffsetInLinesNotBlocked[offset]] = true;
		}
	// Return true for not blocked.
	blocked.flip();
	return blocked;
}
void OpacityFacade::validate(const Area& area) const
{
	const Space& space = area.getSpace();
	Cuboid cuboid = space.boundry();
	for(const Point3D& point : cuboid)
	{
		assert(space.canSeeThrough({point, point}) != m_fullOpacity.query(point));
		assert(space.canSeeThroughFloor({point, point}) != m_floorOpacity.query(point));
	}
}