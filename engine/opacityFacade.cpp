#include "opacityFacade.h"
#include "block.h"
#include "area.h"

OpacityFacade::OpacityFacade(Area& area) : m_area(area)
{
	m_fullOpacity.reserve(area.getBlocks().size());
	m_floorOpacity.reserve(area.getBlocks().size());
	for(const Block& block : area.getBlocks())
		update(block);
}
void OpacityFacade::update(const Block& block)
{
	size_t index = block.getIndex();
	assert(index < m_fullOpacity.size());
	m_fullOpacity[index] = block.canSeeThrough();
	m_floorOpacity[index] = block.canSeeThroughFloor();
}
bool OpacityFacade::isOpaque(size_t index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_fullOpacity[index] == m_area.getBlocks()[index].canSeeThrough());
	return m_fullOpacity[index];
}
bool OpacityFacade::floorIsOpaque(size_t index) const
{
	assert(index < m_fullOpacity.size());
	assert(m_floorOpacity.size() == m_fullOpacity.size());
	assert(m_floorOpacity[index] == m_area.getBlocks()[index].canSeeThroughFloor());
	return m_floorOpacity[index];
}
bool  OpacityFacade::hasLineOfSight(const Block& from, const Block& to) const
{
	assert(!isOpaque(to.getIndex()));
	assert(!isOpaque(from.getIndex()));
	if(&from == &to)
		return true;
	size_t toIndex = to.getIndex();
	size_t currentIndex = from.getIndex();
	//TODO: Would it be faster to calculate x, y, and z from indices and indices from pointer math rather then dereferenceing from and to?
	float x = from.m_x;
	float y = from.m_y;
	float z = from.m_z;
	// Use unsigned types here instead of DistanceInBlocks because delta could be negitive.
	int32_t xDelta = (int32_t)from.m_x - (int32_t)to.m_x;
	int32_t yDelta = (int32_t)from.m_y - (int32_t)to.m_y;
	int32_t zDelta = (int32_t)from.m_z - (int32_t)to.m_z;
	float denominator = std::max({abs(xDelta), abs(yDelta), abs(zDelta)});
	// Normalize delta to a unit vector.
	xDelta /= denominator;
	yDelta /= denominator;
	zDelta /= denominator;
	// Iterate through the line of sight one block at a time untill we hit 'to' or an opaque block.
	// Works by generating coordinates and turning those into vector indices which can be checked in the facade data.
	// Does not load the blocks into memory.
	while(true)
	{
		size_t previousIndex = currentIndex;
		float oldZ = z;
		// Add deltas to coordinates to get next set.
		x += xDelta;
		y += yDelta;
		z += zDelta;
		// Convert coordintates into index.
		currentIndex = m_area.getBlockIndex(x, y, z);
		// Check for success.
		if(currentIndex == toIndex)
			return true;
		// Check for opaque blocks and block features.
		if(!canSeeIntoFrom(previousIndex, currentIndex, oldZ, z))
			return false;
	}
}
bool OpacityFacade::canSeeIntoFrom(size_t previousIndex, size_t currentIndex, DistanceInBlocks oldZ, DistanceInBlocks z) const
{
	assert(!isOpaque(previousIndex));
	if(isOpaque(currentIndex))
		return false;
	if(oldZ == z)
		return true;
	if(oldZ > z)
		return floorIsOpaque(previousIndex);
	else
		return floorIsOpaque(currentIndex);
}
