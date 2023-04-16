#include <algorithm>
#include <cmath>
// Static method.
void VisionRequest::readSteps(std::vector<VisionRequest>::iterator begin, std::vector<VisionRequest>::iterator end)
{
	for(; begin != end; ++begin)
		begin->readStep();
}
VisionRequest::VisionRequest(Actor& a) : m_actor(a) {}
void VisionRequest::readStep()
{
	uint32_t range = m_actor.getVisionRange() * s_maxDistanceVisionModifier;
	for(Actor& actor : m_actor.m_location->m_area->m_locationBuckets.inRange(*m_actor.m_location, range))
	{
		assert(!actor.m_blocks.empty());
		if(m_actor.canSee(actor))
			for(const Block* to : actor.m_blocks)
				if(to->taxiDistance(m_actor.m_location) <= range)
					if(hasLineOfSight(*to, *m_actor.m_location))
					{
						m_actors.insert(&actor);
						continue;
					}
	}
	m_actors.erase(&m_actor);
}
void VisionRequest::writeStep()
{
	m_actor.doVision(m_actors);
}
bool VisionRequest::hasLineOfSight(const Block& to, const Block& from)
{
	if(to.m_area->m_visionCuboidsActive)
		return hasLineOfSightUsingVisionCuboidAndEstablishedAs(to, from);
	else
		return hasLineOfSightUsingEstablishedAs(to, from);
}
bool VisionRequest::hasLineOfSightUsingVisionCuboidAndEstablishedAs(const Block& from, const Block& to)
{
	assert(from.m_area->m_visionCuboidsActive);
	// Iterate line of sight blocks backwards to make the most of the 'established as having' opitimization.
	if(from.m_visionCuboid == to.m_visionCuboid)
	{
		assert(from.m_visionCuboid != nullptr);
		return true;
	}
	if(from == to)
	{
		return true;
	}
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
	m_lineOfSight.clear();
	const Block* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		m_lineOfSight.push_back(block);
		if(block == &from || m_establishedAsHavingLineOfSight.contains(block))
		{
			m_establishedAsHavingLineOfSight.insert(m_lineOfSight.begin(), m_lineOfSight.end());
			m_establishedAsHavingLineOfSight.insert(&to);
			return true;
		}
		previous = block;
	}
}
bool VisionRequest::hasLineOfSightUsingEstablishedAs(const Block& from, const Block& to)
{
	// Iterate line of sight blocks backwards to make the most of the 'established as having' opitimization.
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
	m_lineOfSight.clear();
	const Block* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		m_lineOfSight.push_back(block);
		if(block == &from || m_establishedAsHavingLineOfSight.contains(block))
		{
			m_establishedAsHavingLineOfSight.insert(m_lineOfSight.begin(), m_lineOfSight.end());
			m_establishedAsHavingLineOfSight.insert(&to);
			return true;
		}
		previous = block;
	}
}
// Static method.
bool VisionRequest::hasLineOfSightBasic(const Block& from, const Block& to)
{
	//TODO: Can we reduce repetition here?
	assert(from != to);
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
		Block* block = to.offset(std::floor(xCumulative), std::floor(yCumulative), std::floor(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		if(block == &from)
			return true;
		previous = block;
	}
}
