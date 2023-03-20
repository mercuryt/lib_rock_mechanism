#pragma once
LocationBuckets::LocationBuckets(Area& area) : m_area(area)
{
	m_maxX = m_area.m_sizeX / s_locationBucketSize;
	m_maxY = m_area.m_sizeY / s_locationBucketSize;
	m_maxZ = m_area.m_sizeZ / s_locationBucketSize;
	m_buckets.resize(m_maxX);
	for(uint32_t x = 0; x != m_maxX; ++x)
	{
		m_buckets[x].resize(m_maxY);
		for(uint32_t y = 0; y != m_maxY; ++y)
			m_buckets[x][y].resize(m_maxZ);
	}
}
std::unordered_set<Actor*>* LocationBuckets::getBucketFor(Block* block)
{
	assert(block != nullptr);
	uint32_t bucketX = block->m_x / s_locationBucketSize;
	uint32_t bucketY = block->m_y / s_locationBucketSize;
	uint32_t bucketZ = block->m_z / s_locationBucketSize;
	return &m_buckets[bucketX][bucketY][bucketZ];
}
void LocationBuckets::insert(Actor* actor)
{
	actor->m_location->m_locationBucket->insert(actor);
}
void LocationBuckets::erase(Actor* actor)
{
	actor->m_location->m_locationBucket->erase(actor);
}
void LocationBuckets::update(Actor* actor, Block* oldLocation, Block* newLocation)
{
	if(oldLocation->m_locationBucket == newLocation->m_locationBucket)
		return;
	oldLocation->m_locationBucket->erase(actor);
	newLocation->m_locationBucket->insert(actor);
}
//TODO: Reduce repition, macro?
//TODO: Find out why this throws a linker error
/*
std::unordered_set<Actor*> LocationBuckets::selectActorsInRange(Block* block, int32_t range, ActorRangeSelectors& selectors) const
{
	uint32_t endX = (std::min(block->m_x + range, block->m_area->m_sizeX) / s_locationBucketSize) +1;
	uint32_t startX = std::max(0, (int32_t)block->m_x - range / (int32_t)s_locationBucketSize);
	uint32_t endY = (std::min(block->m_y + range, block->m_area->m_sizeY) / s_locationBucketSize) +1;
	uint32_t startY = std::max(0, (int32_t)block->m_y - range / (int32_t)s_locationBucketSize);
	uint32_t endZ = (std::min(block->m_z + range, block->m_area->m_sizeZ) / s_locationBucketSize) +1;
	uint32_t startZ = std::max(0, (int32_t)block->m_z - range / (int32_t)s_locationBucketSize);
	std::unordered_set<Block*> closed;
	std::unordered_set<Actor*> output;
	for(uint32_t x = startX; x != endX; ++x)
		for(uint32_t y = startY; y != endY; ++y)
			for(uint32_t z = startZ; z != endZ; ++z)
				for(Actor* actor : m_buckets[x][y][z])
					for(Block* b : actor->m_blocks)
						if(b->taxiDistance(block) <= (uint32_t)range && !closed.contains(block))
						{
							closed.insert(block);
							if(selectors.block(block))
								for(auto pair : block->m_actors)
									if(selectors.actor(pair.first))
										output.insert(pair.first);
						}
	return output;
}
*/
void LocationBuckets::processVisionRequest(VisionRequest& visionRequest) const
{
	Block* from = visionRequest.m_actor.m_location;
	assert(from != nullptr);
	assert((int32_t)visionRequest.m_actor.getVisionRange() * (int32_t)s_maxDistanceVisionModifier > 0);
	int32_t range = visionRequest.m_actor.getVisionRange() * s_maxDistanceVisionModifier;
	uint32_t endX = std::min((from->m_x + range + 1) / s_locationBucketSize, m_maxX);
	uint32_t beginX = std::max(0, (int32_t)from->m_x - range) / s_locationBucketSize;
	uint32_t endY = std::min((from->m_y + range + 1) / s_locationBucketSize, m_maxY);
	uint32_t beginY = std::max(0, (int32_t)from->m_y - range) / s_locationBucketSize;
	uint32_t endZ = std::min((from->m_z + range + 1) / s_locationBucketSize, m_maxZ);
	uint32_t beginZ = std::max(0, (int32_t)from->m_z - range) / s_locationBucketSize;
	std::unordered_set<Block*> closed;
	for(uint32_t x = beginX; x != endX; ++x)
		for(uint32_t y = beginY; y != endY; ++y)
			for(uint32_t z = beginZ; z != endZ; ++z)
			{
				assert(x < m_buckets.size());
				assert(y < m_buckets[x].size());
				assert(z < m_buckets[x][y].size());
				const std::unordered_set<Actor*>& bucket = m_buckets[x][y][z];
				for(Actor* actor : bucket)
				{
					assert(!actor->m_blocks.empty());
					for(Block* to : actor->m_blocks)
						if(to->taxiDistance(from) <= (uint32_t)range && !closed.contains(to))
						{
							closed.insert(to);
							if(visionRequest.hasLineOfSightUsingEstablishedAs(to, from))
								for(auto pair : to->m_actors)
									if(visionRequest.m_actor.canSee(pair.first))
										visionRequest.m_actors.insert(pair.first);
						}
				}
			}
	visionRequest.m_actors.erase(&visionRequest.m_actor);
}
