#include <algorithm>
#include <cmath>
// Static method.
void VisionRequest::readSteps(std::vector<VisionRequest>::iterator begin, std::vector<VisionRequest>::iterator end)
{
	for(; begin != end; ++begin)
		begin->readStep();
}
VisionRequest::VisionRequest(DerivedActor& a) : m_actor(a) {}
void VisionRequest::readStep()
{
	m_actor.m_location->m_area->m_locationBuckets.processVisionRequest(*this);
	// This is a more elegant solution then passing the request to location buckets but is also slower.
	 /*
	uint32_t range = m_actor.getVisionRange() * Config::maxDistanceVisionModifier;
	for(DerivedActor& actor : m_actor.m_location->m_area->m_locationBuckets.inRange(*m_actor.m_location, range))
	{
		assert(!actor.m_blocks.empty());
		if(m_actor.canSee(actor))
			for(const DerivedBlock* to : actor.m_blocks)
				if(to->taxiDistance(m_actor.m_location) <= range)
					if(hasLineOfSight(*to, *m_actor.m_location))
					{
						m_actors.insert(&actor);
						continue;
					}
	}
	m_actors.erase(&m_actor);
	*/
}
void VisionRequest::writeStep()
{
	m_actor.doVision(std::move(m_actors));
}
bool VisionRequest::hasLineOfSight(const DerivedBlock& to, const DerivedBlock& from)
{
	if(to.m_area->m_visionCuboidsActive)
		return hasLineOfSightUsingVisionCuboidAndEstablishedAs(to, from);
	else
		return hasLineOfSightUsingEstablishedAs(to, from);
}
bool VisionRequest::hasLineOfSightUsingVisionCuboidAndEstablishedAs(const DerivedBlock& from, const DerivedBlock& to)
{
	// Iterate line of sight blocks backwards to make the most of the 'established as having' opitimization.
	assert(from.m_area->m_visionCuboidsActive);
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
	const DerivedBlock* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		DerivedBlock* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
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
bool VisionRequest::hasLineOfSightUsingEstablishedAs(const DerivedBlock& from, const DerivedBlock& to)
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
	const DerivedBlock* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		DerivedBlock* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
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
bool VisionRequest::hasLineOfSightUsingVisionCuboid(const DerivedBlock& from, const DerivedBlock& to)
{
	assert(from.m_area->m_visionCuboidsActive);
	if(from.m_visionCuboid == to.m_visionCuboid)
	{
		assert(from.m_visionCuboid != nullptr);
		return true;
	}
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
	const DerivedBlock* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		DerivedBlock* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		if(block == &from)
			return true;
		previous = block;
	}
}
// Static method.
bool VisionRequest::hasLineOfSightBasic(const DerivedBlock& from, const DerivedBlock& to)
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
	const DerivedBlock* previous = &to;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		DerivedBlock* block = to.offset(std::round(xCumulative), std::round(yCumulative), std::round(zCumulative));
		if(!block->canSeeThroughFrom(*previous))
			return false;
		if(block == &from)
			return true;
		previous = block;
	}
}
