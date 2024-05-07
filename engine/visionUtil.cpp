#include "visionUtil.h"
#include "block.h"
#include "area.h"
bool visionUtil::hasLineOfSightUsingVisionCuboid(const Block& from, const Block& to)
{
	assert(from.m_area->m_visionCuboidsActive);
	if(from.m_visionCuboid == to.m_visionCuboid)
	{
		assert(from.m_visionCuboid != nullptr);
		return true;
	}
	//TODO: Can we reduce repetition here?
	if(from == to)
		return true;
	int32_t xDiff = (int32_t)from.m_x - (int32_t)to.m_x;
	int32_t yDiff = (int32_t)from.m_y - (int32_t)to.m_y;
	int32_t zDiff = (int32_t)from.m_z - (int32_t)to.m_z;
	std::array<int32_t, 3> diffs = {abs(xDiff), abs(yDiff), abs(zDiff)};
	float denominator = *std::ranges::max_element(diffs);
	float xDiffNormalized = xDiff / denominator;
	float yDiffNormalized = yDiff / denominator;
	float zDiffNormalized = zDiff / denominator;
	float xCumulative = 0;
	float yCumulative = 0;
	float zCumulative = 0;
	const Block* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		if(block == &from)
			return true;
		previous = block;
	}
}
bool visionUtil::hasLineOfSightBasic(const Block& from, const Block& to)
{
	//TODO: Can we reduce repetition here?
	if(from == to)
		return true;
	int32_t xDiff = (int32_t)from.m_x - (int32_t)to.m_x;
	int32_t yDiff = (int32_t)from.m_y - (int32_t)to.m_y;
	int32_t zDiff = (int32_t)from.m_z - (int32_t)to.m_z;
	std::array<int32_t, 3> diffs = {abs(xDiff), abs(yDiff), abs(zDiff)};
	float denominator = *std::ranges::max_element(diffs);
	float xDiffNormalized = xDiff / denominator;
	float yDiffNormalized = yDiff / denominator;
	float zDiffNormalized = zDiff / denominator;
	float xCumulative = 0;
	float yCumulative = 0;
	float zCumulative = 0;
	const Block* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		// TODO: create an offsetNotNull to prevent an unnessesary branch.
		Block* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		if(block == &from)
			return true;
		previous = block;
	}
}
