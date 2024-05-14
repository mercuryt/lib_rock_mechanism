#include "opacityFacade.h"
#include "block.h"
#include "area.h"
#include "types.h"

OpacityFacade::OpacityFacade(Area& area) : m_area(area) 
{
	assert(m_area.getBlocks().size());
	m_fullOpacity.resize(m_area.getBlocks().size());
	m_floorOpacity.resize(m_area.getBlocks().size());
}
void OpacityFacade::initalize()
{
	for(const Block& block : m_area.getBlocks())
		update(block);
}
void OpacityFacade::update(const Block& block)
{
	BlockIndex index = m_area.getBlockIndex(block);
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	m_fullOpacity.at(index) = !block.canSeeThrough();
	m_floorOpacity.at(index) = !block.canSeeThroughFloor();
}
void OpacityFacade::validate() const
{
	for(const Block& block : m_area.getBlocks())
	{
		assert(block.canSeeThrough() != m_fullOpacity.at(m_area.getBlockIndex(block)));
		assert(block.canSeeThroughFloor() != m_floorOpacity.at(m_area.getBlockIndex(block)));
	}
}
bool OpacityFacade::isOpaque(BlockIndex index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_fullOpacity.at(index) != m_area.getBlocks().at(index).canSeeThrough());
	return m_fullOpacity.at(index);
}
bool OpacityFacade::floorIsOpaque(BlockIndex index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	assert(m_floorOpacity.at(index) != m_area.getBlocks().at(index).canSeeThroughFloor());
	return m_floorOpacity.at(index);
}
bool  OpacityFacade::hasLineOfSight(const Block& from, const Block& to) const
{
	BlockIndex fromIndex = m_area.getBlockIndex(from);
	Point3D fromCoords = m_area.getCoordinatesForIndex(fromIndex);
	BlockIndex toIndex = m_area.getBlockIndex(to);
	Point3D toCoords = m_area.getCoordinatesForIndex(toIndex);
	return hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords);
}
bool  OpacityFacade::hasLineOfSight(BlockIndex fromIndex, Point3D fromCoords, BlockIndex toIndex, Point3D toCoords) const
{
	assert(!isOpaque(toIndex));
	assert(!isOpaque(fromIndex));
	if(fromIndex == toIndex)
		return true;
	BlockIndex currentIndex = fromIndex;
	//TODO: Would it be faster to use fixed percision number types? create low percision fixed via bitshift?
	float x = fromCoords.x;
	float y = fromCoords.y;
	float z = fromCoords.z;
	// Use unsigned types here instead of DistanceInBlocks because delta could be negitive.
	int32_t xDelta = (int32_t)toCoords.x - (int32_t)fromCoords.x;
	int32_t yDelta = (int32_t)toCoords.y - (int32_t)fromCoords.y;
	int32_t zDelta = (int32_t)toCoords.z - (int32_t)fromCoords.z;
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
		zInt = std::round(z);
		// Convert coordintates into index.
		currentIndex = m_area.getBlockIndex({(DistanceInBlocks)std::round(x), (DistanceInBlocks)std::round(y), zInt});
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
