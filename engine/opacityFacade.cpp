#include "opacityFacade.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"

OpacityFacade::OpacityFacade(Area& area) : m_area(area) { }
void OpacityFacade::initalize()
{
	assert(m_area.getBlocks().size());
	m_fullOpacity.resize(m_area.getBlocks().size());
	m_floorOpacity.resize(m_area.getBlocks().size());
	for(BlockIndex block : m_area.getBlocks().getAll())
		update(block);
}
void OpacityFacade::update(BlockIndex index)
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
	for(BlockIndex block : m_area.getBlocks().getAll())
	{
		assert(blocks.canSeeThrough(block) != m_fullOpacity[block]);
		assert(blocks.canSeeThroughFloor(block) != m_floorOpacity[block]);
	}
}
bool OpacityFacade::isOpaque(BlockIndex index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_fullOpacity[index] != m_area.getBlocks().canSeeThrough(index));
	return m_fullOpacity[index];
}
bool OpacityFacade::floorIsOpaque(BlockIndex index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	assert(m_floorOpacity[index] != m_area.getBlocks().canSeeThroughFloor(index));
	return m_floorOpacity[index];
}
bool  OpacityFacade::hasLineOfSight(BlockIndex from, BlockIndex to) const
{
	Point3D fromCoords = m_area.getBlocks().getCoordinates(from);
	Point3D toCoords = m_area.getBlocks().getCoordinates(to);
	return hasLineOfSight(from, fromCoords, to, toCoords);
}
bool  OpacityFacade::hasLineOfSight(BlockIndex fromIndex, Point3D fromCoords, BlockIndex toIndex, Point3D toCoords) const
{
	assert(!isOpaque(toIndex));
	assert(!isOpaque(fromIndex));
	if(fromIndex == toIndex)
		return true;
	BlockIndex currentIndex = fromIndex;
	//TODO: Would it be faster to use fixed percision number types?
	float x = fromCoords.x.get();
	float y = fromCoords.y.get();
	float z = fromCoords.z.get();
	// Use unsigned types here instead of DistanceInBlocks because delta could be negitive.
	int32_t xDelta = (toCoords.x - fromCoords.x).get();
	int32_t yDelta = (toCoords.y - fromCoords.y).get();
	int32_t zDelta = (toCoords.z - fromCoords.z).get();
	float denominator = std::max({abs(xDelta), abs(yDelta), abs(zDelta)});
	// Normalize delta to a unit vector.
	float xDeltaNormalized = xDelta / denominator;
	float yDeltaNormalized = yDelta / denominator;
	float zDeltaNormalized = zDelta / denominator;
	// Iterate through the line of sight one block at a time untill we hit 'to' or an opaque block.
	// Works by generating coordinates and turning those into vector indices which can be checked in the facade data.
	DistanceInBlocks zInt = fromCoords.z;
	while(true)
	{
		BlockIndex previousIndex = currentIndex;
		DistanceInBlocks oldZ = zInt;
		// Add deltas to coordinates to get next set.
		x += xDeltaNormalized;
		y += yDeltaNormalized;
		z += zDeltaNormalized;
		// Round to integer and store for use as oldZ next iteration.
		zInt = DistanceInBlocks::create(std::round(z));
		// Convert coordintates into index.
		currentIndex = m_area.getBlocks().getIndex({DistanceInBlocks::create(std::round(x)), DistanceInBlocks::create(std::round(y)), zInt});
		// Check for opaque blocks and block features.
		if(!canSeeIntoFrom(previousIndex, currentIndex, oldZ, zInt))
			return false;
		// Check for success.
		if(currentIndex == toIndex)
			return true;
	}
}
bool OpacityFacade::canSeeIntoFrom(BlockIndex previousIndex, BlockIndex currentIndex, DistanceInBlocks oldZ, DistanceInBlocks z) const
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
