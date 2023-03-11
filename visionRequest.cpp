#include <algorithm>

VisionRequest::VisionRequest(Actor* a) : m_actor(a) {}
void VisionRequest::readStep()
{
	int32_t visionRange = m_actor->getVisionRange() * MAX_VISION_DISTANCE_MODIFIER;
	Block* location = m_actor->m_location;
	Area* area = location->m_area;
	uint32_t xBegin = std::max(0, (int32_t)location->m_x - visionRange);
	uint32_t xEnd = std::min((int32_t)area->m_sizeX, (int32_t)location->m_x + visionRange);
	uint32_t yBegin = std::max(0, (int32_t)location->m_y - visionRange);
	uint32_t yEnd = std::min((int32_t)area->m_sizeY, (int32_t)location->m_y + visionRange);
	uint32_t zBegin = std::max(0, (int32_t)location->m_z - visionRange);
	uint32_t zEnd = std::min((int32_t)area->m_sizeZ, (int32_t)location->m_z + visionRange);
	for(uint32_t x = xBegin; x != xEnd; x++)
		for(uint32_t y = yBegin; y != yEnd; y++)
			for(uint32_t z = zBegin; z != zEnd; z++)
			{
				Block& b = area->m_blocks[x][y][z];
				if(location == &b || 
					(b.taxiDistance(location) * b.visionDistanceModifier() <= visionRange && hasLineOfSight(location, &b))
				  )
					for(auto& pair : b.m_actors)
						if(m_actor->canSee(pair.first))
							m_actors.insert(pair.first);	
			}
	m_actors.erase(m_actor);
}
void VisionRequest::writeStep()
{
	m_actor->doVision(m_actors);
}
bool VisionRequest::hasLineOfSight(Block* from, Block* to)
{
	// Same as hasLineOfSight but returns blocks visited.
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
	m_lineOfSight.clear();
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* b = to->offset(std::floor(xCumulative), std::floor(yCumulative), std::floor(zCumulative));
		if(!b->canSeeThrough())
			return false;
		m_lineOfSight.push_back(b);
		if(b == from || m_establishedAsHavingLineOfSight.contains(b))
		{
			m_establishedAsHavingLineOfSight.insert(m_lineOfSight.begin(), m_lineOfSight.end());
			return true;
		}
	}
}
