#include "opacityFacade.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"
#include "lineGenerator.h"

OpacityFacade::OpacityFacade(Area& area) : m_area(area) { }
void OpacityFacade::initalize()
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.size());
	m_fullOpacity.resize(blocks.size());
	m_floorOpacity.resize(blocks.size());
	Cuboid cuboid = blocks.getAll();
	for(const BlockIndex& block : cuboid.getView(blocks))
		update(block);
}
void OpacityFacade::update(const BlockIndex& index)
{
	Blocks& blocks = m_area.getBlocks();
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	m_fullOpacity.set(index, !blocks.canSeeThrough(index));
	m_floorOpacity.set(index, !blocks.canSeeThroughFloor(index));
}
void OpacityFacade::validate() const
{
	Blocks& blocks = m_area.getBlocks();
	Cuboid cuboid = blocks.getAll();
	for(const BlockIndex& block : cuboid.getView(blocks))
	{
		assert(blocks.canSeeThrough(block) != m_fullOpacity[block]);
		assert(blocks.canSeeThroughFloor(block) != m_floorOpacity[block]);
	}
}
bool OpacityFacade::isOpaque(const BlockIndex& index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_fullOpacity[index] != m_area.getBlocks().canSeeThrough(index));
	return m_fullOpacity[index];
}
bool OpacityFacade::floorIsOpaque(const BlockIndex& index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	assert(m_floorOpacity[index] != m_area.getBlocks().canSeeThroughFloor(index));
	return m_floorOpacity[index];
}
bool OpacityFacade::hasLineOfSight(const BlockIndex& from, const BlockIndex& to) const
{
	if(from == to)
		return true;
	Point3D fromCoords = m_area.getBlocks().getCoordinates(from);
	Point3D toCoords = m_area.getBlocks().getCoordinates(to);
	return hasLineOfSight(from, fromCoords, toCoords);
}
bool OpacityFacade::hasLineOfSight(const BlockIndex& fromIndex, Point3D fromCoords, Point3D toCoords) const
{
	const BlockIndex& toIndex = m_area.getBlocks().getIndex(toCoords);
	assert(!isOpaque(toIndex));
	assert(!isOpaque(fromIndex));
	assert(fromCoords != toCoords);
	BlockIndex currentIndex = fromIndex;
	BlockIndex previousIndex;
	Blocks& blocks = m_area.getBlocks();
	bool result = true;
	DistanceInBlocks previousZ;
	DistanceInBlocks currentZ = fromCoords.z;
	forEachOnLine(fromCoords, toCoords, [&](int x, int y, int z){
		previousIndex = currentIndex;
		previousZ = currentZ;
		currentIndex = blocks.getIndex_i(x, y, z);
		currentZ = DistanceInBlocks::create(z);
		if(!canSeeIntoFrom(previousIndex, currentIndex, previousZ, currentZ))
		{
			result = false;
			return false;
		}
		// Check for success.
		if(currentIndex == toIndex)
		{
			result = true;
			return false;
		}
		// Not done and not blocked, continue.
		return true;
	});
	return result;
}
bool OpacityFacade::canSeeIntoFrom(const BlockIndex& previousIndex, const BlockIndex& currentIndex, const DistanceInBlocks& oldZ, const DistanceInBlocks& z) const
{
	assert(!isOpaque(previousIndex));
	if(isOpaque(currentIndex))
		return false;
	if(oldZ == z)
		return true;
	if(oldZ > z)
		return !floorIsOpaque(previousIndex);
	else
		return !floorIsOpaque(currentIndex);
}
