#include <algorithm>
#include <cmath>
VisionRequest::VisionRequest(Actor& a) : m_actor(a) {}
void VisionRequest::readStep()
{
	m_actor.m_location->m_area->m_locationBuckets.processVisionRequest(*this);
}
void VisionRequest::writeStep()
{
	m_actor.doVision(m_actors);
}
bool VisionRequest::hasLineOfSightUsingEstablishedAs(Block* from, Block* to)
{
	// Iterate line of sight blocks backwards to make the most of the 'established as having' opitimization.
	if(from == to)
		return true;
	int32_t xDiff = (int32_t)from->m_x - (int32_t)to->m_x;
	int32_t yDiff = (int32_t)from->m_y - (int32_t)to->m_y;
	int32_t zDiff = (int32_t)from->m_z - (int32_t)to->m_z;
	std::array<int32_t, 3> diffs = {abs(xDiff), abs(yDiff), abs(zDiff)};
	float denominator = *std::ranges::max_element(diffs);
	float xDiffNormalized = xDiff / denominator;
	float yDiffNormalized = yDiff / denominator;
	float zDiffNormalized = zDiff / denominator;
	float xCumulative = 0;
	float yCumulative = 0;
	float zCumulative = 0;
	m_lineOfSight.clear();
	Block* previous = to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* block = to->offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(previous))
			return false;
		m_lineOfSight.push_back(block);
		if(block == from || m_establishedAsHavingLineOfSight.contains(block))
		{
			m_establishedAsHavingLineOfSight.insert(m_lineOfSight.begin(), m_lineOfSight.end());
			m_establishedAsHavingLineOfSight.insert(to);
			return true;
		}
		previous = block;
	}
}

bool VisionRequest::hasLineOfSight(Block* from, Block* to)
{
	//TODO: Can we reduce repetition here?
	assert(from != to);
	int32_t xDiff = (int32_t)from->m_x - (int32_t)to->m_x;
	int32_t yDiff = (int32_t)from->m_y - (int32_t)to->m_y;
	int32_t zDiff = (int32_t)from->m_z - (int32_t)to->m_z;
	std::array<int32_t, 3> diffs = {abs(xDiff), abs(yDiff), abs(zDiff)};
	float denominator = *std::ranges::max_element(diffs);
	float xDiffNormalized = xDiff / denominator;
	float yDiffNormalized = yDiff / denominator;
	float zDiffNormalized = zDiff / denominator;
	float xCumulative = 0;
	float yCumulative = 0;
	float zCumulative = 0;
	Block* previous = to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* b = to->offset(std::floor(xCumulative), std::floor(yCumulative), std::floor(zCumulative));
		if(!b->canSeeThroughFrom(previous))
			return false;
		if(b == from)
			return true;
		previous = b;
	}
}
