/*
* A game map.
*/

#pragma once

baseArea::baseArea(uint32_t x, uint32_t y, uint32_t z) : m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_visionCacheVersion(0), m_routeCacheVersion(0)
{
	// build m_blocks
	m_blocks.resize(x);
	for(uint32_t ix = 0; ix < x; ++ix)
	{
		m_blocks[ix].resize(y);
		for(uint32_t iy = 0; iy < y; ++iy)
		{
			m_blocks[ix][iy].resize(z);
			for(uint32_t iz = 0; iz < z; ++iz)
				m_blocks[ix][iy][iz].setup(static_cast<Area*>(this), ix, iy, iz);
		}
	}
	// record adjacent m_blocks
	for(uint32_t ix = 0; ix < x; ++ix)
		for(uint32_t iy = 0; iy < y; ++iy)
			for(uint32_t iz = 0; iz < z; ++iz)
				m_blocks[ix][iy][iz].recordAdjacent();
	//TODO: record diagonal?
}
void baseArea::readStep()
{ 
	//TODO: Count tasks dispatched and finished instead of pool.wait_for_tasks so we can do multiple areas simultaniously in one pool.
	// Process vision, generate and push_task requests for every actor in current bucket.
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(s_step))
	{
		m_visionRequestQueue.emplace_back(actor);
		VisionRequest& visionRequest = m_visionRequestQueue.back();
		s_pool.push_task([&](){visionRequest.readStep(); });
	}
	// calculate cave in
	s_pool.push_task([&](){stepCaveInRead();});
	// calculate flow
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		s_pool.push_task([&](){ fluidGroup->readStep(); });
	// calculate routes
	for(RouteRequest& routeRequest : m_routeRequestQueue)
		s_pool.push_task([&](){ routeRequest.readStep(); });
}
void baseArea::writeStep()
{ 
	s_pool.wait_for_tasks();
	// Apply cave in.
	if(!m_caveInData.empty())
	{
		stepCaveInWrite();
		expireRouteCache();
		expireVisionCache();
	}
	// Apply flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		fluidGroup->writeStep();
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_stable || fluidGroup->m_destroy; });
	std::erase_if(m_fluidGroups, [](FluidGroup& fluidGroup){ return fluidGroup.m_destroy; });
	// Apply fluid merge.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		fluidGroup->mergeStep();
	std::erase_if(m_fluidGroups, [](FluidGroup& fluidGroup){ return fluidGroup.m_merged; });
	// Apply fluid split.
	std::vector<FluidGroup*> newlySplit;
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		fluidGroup->splitStep(newlySplit);
	m_unstableFluidGroups.insert(newlySplit.begin(), newlySplit.end());
	// Apply routes.
	for(RouteRequest& routeRequest : m_routeRequestQueue)
		routeRequest.writeStep();
	// Apply vision request results.
	for(VisionRequest& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
	m_visionRequestQueue.clear();
	// Do scheduled events.
	exeuteScheduledEvents(s_step);
}
void baseArea::registerActor(Actor* actor)
{
	m_visionBuckets.add(actor);
}
void baseArea::unregisterActor(Actor* actor)
{
	m_visionBuckets.remove(actor);
}
void baseArea::scheduleMove(Actor* actor)
{
	Block* block = *(actor->m_routeIter);
	uint32_t stepsToMove = block->moveCost(actor->m_moveType, actor->m_location) / actor->getSpeed();
	MoveEvent* moveEvent = new MoveEvent(s_step + stepsToMove, actor);
	scheduleEvent(moveEvent);
}
void baseArea::registerRouteRequest(Actor* actor)
{
	m_routeRequestQueue.emplace_back(actor);
}
FluidGroup* baseArea::createFluidGroup(const FluidType* fluidType, std::unordered_set<Block*>& blocks, bool checkMerge)
{
	m_fluidGroups.emplace_back(fluidType, blocks, static_cast<Area*>(this), checkMerge);
	m_unstableFluidGroups.insert(&m_fluidGroups.back());
	return &m_fluidGroups.back();
}
void baseArea::removeFluidGroup(FluidGroup* fluidGroup)
{
	m_unstableFluidGroups.erase(fluidGroup);
	std::erase_if(m_fluidGroups, [&](FluidGroup& fg){ return &fg == fluidGroup; });
}
void baseArea::expireVisionCache(){++m_visionCacheVersion;}
void baseArea::expireRouteCache(){++m_routeCacheVersion;}

