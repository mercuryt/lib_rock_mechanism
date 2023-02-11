#include <algorithm>

VisionRequest::VisionRequest(Actor* a) : m_actor(a) {}

void VisionRequest::readStep()
{
	uint32_t visionRange = m_actor->getVisionRange();
	std::vector<Block*>* blocks;
	if(m_actor->m_location->m_visionCacheVersion == m_actor->m_location->m_area->m_visionCacheVersion)
		blocks = &m_actor->m_location->m_visionCache;
	else
	{
		m_blocks = getVisibleBlocks(CACHEABLE_VISION_RANGE);
		blocks = &m_blocks;
	}
	if(visionRange > CACHEABLE_VISION_RANGE)
		m_actors = getVisibleActors(visionRange, blocks);
	else
		for(Block* block : *blocks)
			if(visionRange == CACHEABLE_VISION_RANGE || block->distance(m_actor->m_location) <= visionRange)
				for(auto& [actor, volume] : block->m_actors)
					if(actor->isVisible(m_actor))
						m_actors.insert(actor);
}

void VisionRequest::writeStep()
{
	if(!m_blocks.empty())
	{
		m_actor->m_location->m_visionCache = m_blocks;
		m_actor->m_location->m_visionCacheVersion = m_actor->m_location->m_area->m_visionCacheVersion;
	}
	m_actor->doVision(m_actors);
}

std::vector<Block*> VisionRequest::getVisibleBlocks(uint32_t range) const
{
	// Add start block to current.
	// For number in range.
	// Add adjacent blocks to next and to output if not canSeeThrough and has line of sight to start or passes throgh another block on the way to start which is already in output.
	// Make next new current.
	// Clear next.
	// Return output.

	std::unordered_set<Block*> output;
	std::vector<Block*> currentBlocks;
	currentBlocks.push_back(m_actor->m_location);
	std::vector<Block*> nextBlocks;
	std::unordered_set<Block*> closed;
	for(;range != 0; --range)
	{
		for(Block* block : currentBlocks)
		{
			closed.insert(block);
			for(Block* adjacent : block->m_adjacents)
			{
				if(adjacent->canSeeThrough() && !closed.contains(adjacent) && hasLineOfSight(adjacent, block, output))
				{
					nextBlocks.push_back(adjacent);
					output.insert(adjacent);
				}
			}
		}	
		if(nextBlocks.empty())
			break;
		std::swap(nextBlocks, currentBlocks);
		nextBlocks.clear();
	}
	return std::vector<Block*>(output.begin(), output.end());
}

std::unordered_set<Actor*> VisionRequest::getVisibleActors(uint32_t range, std::vector<Block*>* preseedEstablished)
{
	// Iterate blocks in range.
	// Accumulate blocks with actors in openLIst.
	// Sort openList by inverse distance.
	// Starting at each block look for line of sight back to location.
	// Arriving at m_actor->m_location or an established block counts as sucess.
	// Accumulate other blocks found in line of sight.
	// If line of sight found all blocks are moved from openList to established and all actors are added to output.
	// Pass a vector of blocks to preseed established.
	Block* location = m_actor->m_location;
	Area* area = location->m_area;
	std::vector<Block*> blocksContainingActors;
	for(uint32_t x = std::max(0u, location->m_x) - range; x != std::min(area->m_sizeX, location->m_x + range);x++)
		for(uint32_t y = std::max(0u, location->m_y) - range; y != std::min(area->m_sizeY, location->m_y + range);y++)
			for(uint32_t z = std::max(0u, location->m_z) - range; z != std::min(area->m_sizeZ, location->m_z + range);z++)
			{
				Block& b = area->m_blocks[x][y][z];
				if(!b.m_actors.empty() && b.taxiDistance(location) <= range)
					blocksContainingActors.push_back(&b);
			}
	//TODO: Profile if sorting is worth it.
	auto condition = [&](Block* a, Block* b){ return location->taxiDistance(a) < location->taxiDistance(b); };
	std::sort(blocksContainingActors.begin(), blocksContainingActors.end(), condition);
	std::unordered_set<Block*> establishedAsHavingLineOfSight(preseedEstablished->begin(), preseedEstablished->end()); //TODO: Profile if preseed and/or existance of established are worth it.
	std::unordered_set<Actor*> output;
	for(Block* block : blocksContainingActors)
	{
		if(establishedAsHavingLineOfSight.contains(block))
			recordActorsInBlock(block);
		else
		{
			std::stack<Block*> blocks = getLineOfSight(block, location, establishedAsHavingLineOfSight);
			while(!blocks.empty())
			{
				Block*& b = blocks.top();
				recordActorsInBlock(b);
				establishedAsHavingLineOfSight.insert(b);
				blocks.pop();
			}
		}
	}
	return output;
}

bool VisionRequest::hasLineOfSight(Block* from, Block* to, std::unordered_set<Block*> establishedAsHavingLineOfSight) const
{
	// Convert x, y and z difference into slope by dividing each by the largest.
	// Start at 'to' and go backwards to 'from' to allow established-as-having optimization.
	// Take tiles in the direction of the largest difference until the second largest adds up to one.
	// So if the xyz difference slope is 1,0.4,0, then we take 3 steps on the x axis for ever one step on the y axis.
	// And if it's 1, 0.4, 0.2 then we take 5 steps on the x axis and 3 steps on the y axis for each one step on the z axis.
	// If any step is into a not canSeeThrough block then return false.
	// If any step is into an established block or from block then return true.
	uint32_t xDiff = to->m_x - from->m_x;
	uint32_t yDiff = to->m_y - from->m_y;
	uint32_t zDiff = to->m_z - from->m_z;
	float denominator = std::max(xDiff, yDiff, zDiff);
	float xDiffNormalized = xDiff / denominator;
	float yDiffNormalized = yDiff / denominator;
	float zDiffNormalized = zDiff / denominator;
	float xCumulative = 0;
	float yCumulative = 0;
	float zCumulative = 0;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* b = to->offset(std::floor(xCumulative), std::floor(yCumulative), std::floor(zCumulative));
		if(b == from)
			return true;
		if(!b->canSeeThrough())
			return false;
		if(establishedAsHavingLineOfSight.contains(b))
			return true;
	}
}

std::stack<Block*> VisionRequest::getLineOfSight(Block* from, Block* to, std::unordered_set<Block*> establishedAsHavingLineOfSight) const
{
	// Same as hasLineOfSight but returns blocks visited.
	// TODO: reduce repetition?
	uint32_t xDiff = to->m_x - from->m_x;
	uint32_t yDiff = to->m_y - from->m_y;
	uint32_t zDiff = to->m_z - from->m_z;
	float denominator = std::max(xDiff, yDiff, zDiff);
	float xDiffNormalized = xDiff / denominator;
	float yDiffNormalized = yDiff / denominator;
	float zDiffNormalized = zDiff / denominator;
	float xCumulative = 0;
	float yCumulative = 0;
	float zCumulative = 0;
	std::stack<Block*> output;
	while(true)
	{
		xCumulative += xDiffNormalized;
		yCumulative += yDiffNormalized;
		zCumulative += zDiffNormalized;
		Block* b = to->offset(std::floor(xCumulative), std::floor(yCumulative), std::floor(zCumulative));
		if(!b->canSeeThrough())
			return std::stack<Block*>();
		output.push(b);
		if(b == from || establishedAsHavingLineOfSight.contains(b))
			return output;
	}
}

void VisionRequest::recordActorsInBlock(Block* block)
{
	for(auto& pair : block->m_actors)
		if(pair.first->isVisible(m_actor))
			m_actors.insert(pair.first);
}
