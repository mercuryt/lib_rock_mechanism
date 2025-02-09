#include "vision/opacityFacade.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"
#include "lineGenerator.h"

OpacityFacade::OpacityFacade(Area& area) : m_area(area) { }
void OpacityFacade::initalize()
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.size() != 0);
	uint size = blocks.getChunkedSize();
	m_fullOpacity.resize(size);
	m_floorOpacity.resize(size);
	Cuboid cuboid = blocks.getAll();
	for(const BlockIndex& block : cuboid.getView(blocks))
		update(block);
}
void OpacityFacade::update(const BlockIndex& index)
{
	Blocks& blocks = m_area.getBlocks();
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	const BlockIndexChunked chunkedIndex = blocks.getIndexChunked(blocks.getCoordinates(index));
	m_fullOpacity.set(chunkedIndex, !blocks.canSeeThrough(index));
	m_floorOpacity.set(chunkedIndex, !blocks.canSeeThroughFloor(index));
}
void OpacityFacade::validate() const
{
	Blocks& blocks = m_area.getBlocks();
	Cuboid cuboid = blocks.getAll();
	for(const BlockIndex& block : cuboid.getView(blocks))
	{
		[[maybe_unused]] BlockIndexChunked chunkedIndex = blocks.getIndexChunked(blocks.getCoordinates(block));
		assert(blocks.canSeeThrough(block) != m_fullOpacity[chunkedIndex]);
		assert(blocks.canSeeThroughFloor(block) != m_floorOpacity[chunkedIndex]);
	}
}
bool OpacityFacade::isOpaque(const BlockIndexChunked& index) const
{
	assert(index < m_fullOpacity.size());
	return m_fullOpacity[index];
}
bool OpacityFacade::floorIsOpaque(const BlockIndexChunked& index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	return m_floorOpacity[index];
}
bool OpacityFacade::hasLineOfSight(const BlockIndex& from, const BlockIndex& to) const
{
	if(from == to)
		return true;
	Point3D fromCoords = m_area.getBlocks().getCoordinates(from);
	Point3D toCoords = m_area.getBlocks().getCoordinates(to);
	return hasLineOfSight(fromCoords, toCoords);
}
bool OpacityFacade::hasLineOfSight(const Point3D& fromCoords, const Point3D& toCoords) const
{
	const BlockIndexChunked toIndex = m_area.getBlocks().getIndexChunked(toCoords);
	assert(!isOpaque(toIndex));
	assert(fromCoords != toCoords);
	Blocks& blocks = m_area.getBlocks();
	assert(!isOpaque(blocks.getIndexChunked(fromCoords)));
	BlockIndexChunked currentIndex = blocks.getIndexChunked(fromCoords);
	BlockIndexChunked previousIndex;
	bool result = true;
	DistanceInBlocks previousZ;
	DistanceInBlocks currentZ = fromCoords.z();
	forEachOnLine(fromCoords, toCoords, [&](int x, int y, int z){
		previousIndex = currentIndex;
		previousZ = currentZ;
		currentIndex = blocks.getIndexChunked(Point3D::create(x, y, z));
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
bool OpacityFacade::canSeeIntoFrom(const BlockIndexChunked& previousIndex, const BlockIndexChunked& currentIndex, const DistanceInBlocks& oldZ, const DistanceInBlocks& z) const
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
