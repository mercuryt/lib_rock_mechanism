/*
* A game map.
*/

#pragma once
#include "area.h"
#include <algorithm>

baseArea::baseArea(uint32_t x, uint32_t y, uint32_t z) :
	m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_locationBuckets(static_cast<Area&>(*this)), m_routeCacheVersion(0)
{
	// build m_blocks
	m_blocks.resize(m_sizeX);
	for(uint32_t x = 0; x < m_sizeX; ++x)
	{
		m_blocks[x].resize(m_sizeY);
		for(uint32_t y = 0; y < m_sizeY; ++y)
		{
			m_blocks[x][y].resize(m_sizeZ);
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				m_blocks[x][y][z].setup(static_cast<Area*>(this), x, y, z);
		}
	}
	// record adjacent m_blocks
	for(uint32_t x = 0; x < m_sizeX; ++x)
		for(uint32_t y = 0; y < m_sizeY; ++y)
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				m_blocks[x][y][z].recordAdjacent();
}
baseArea::~baseArea()
{
	resetScheduledEvents();
}
void baseArea::readStep()
{ 
	//TODO: Count tasks dispatched and finished instead of pool.wait_for_tasks so we can do multiple areas simultaniously in one pool.
	// Process vision, generate and push_task requests for every actor in current bucket.
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(s_step))
	{
		m_visionRequestQueue.emplace_back(*actor);
		VisionRequest* visionRequest = &m_visionRequestQueue.back();
		s_pool.push_task([=](){ visionRequest->readStep(); });
	}
	// Calculate cave in.
	s_pool.push_task([&](){ stepCaveInRead(); });
	// Calculate flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		s_pool.push_task([=](){ fluidGroup->readStep(); });
	// Calculate routes.
	for(RouteRequest& routeRequest : m_routeRequestQueue)
	{
		RouteRequest* routeRequestPointer = &routeRequest;
		s_pool.push_task([=](){ routeRequestPointer->readStep(); });
	}
}
void baseArea::writeStep()
{ 
	s_pool.wait_for_tasks();
	// Apply flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		fluidGroup->writeStep();
		validateAllFluidGroups();
	}
	// Resolve overfull, diagonal seep, and mist.
	// Make vector of unstable so we can iterate it while modifing the original.
	std::vector<FluidGroup*> unstable(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable)
	{
		fluidGroup->afterWriteStep();
		fluidGroup->validate();
	}
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_stable || fluidGroup->m_disolved || fluidGroup->m_destroy; });
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		fluidGroup->mergeStep();
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	// Apply fluid split.
	std::vector<FluidGroup*> unstable2(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable2)
		fluidGroup->splitStep();
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	std::unordered_set<FluidGroup*> toErase;
	for(FluidGroup& fluidGroup : m_fluidGroups)
	{
		if(fluidGroup.m_excessVolume <= 0 && fluidGroup.m_drainQueue.m_set.size() == 0)
		{
			fluidGroup.m_destroy = true;
			assert(!fluidGroup.m_disolved);
		}
		if(fluidGroup.m_destroy || fluidGroup.m_merged || fluidGroup.m_disolved || fluidGroup.m_stable)
			m_unstableFluidGroups.erase(&fluidGroup);
		if(fluidGroup.m_destroy || fluidGroup.m_merged)
		{
			toErase.insert(&fluidGroup);
			assert(fluidGroup.m_drainQueue.m_set.empty());
		}
	}
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(toErase);
	m_fluidGroups.remove_if([&](FluidGroup& fluidGroup){ return toErase.contains(&fluidGroup); });
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(toErase);
	for(const FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		bool found = false;
		for(FluidGroup& fg : m_fluidGroups)
			if(&fg == fluidGroup)
			{
				found = true;
				continue;
			}
		assert(found);
	}
	// Apply fluid merge.
	// Apply cave in.
	if(!m_caveInData.empty())
		stepCaveInWrite();
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// Apply routes.
	for(RouteRequest& routeRequest : m_routeRequestQueue)
		routeRequest.writeStep();
	m_routeRequestQueue.clear();
	// If there is any unstable groups expire route caches.
	// TODO: Be more selective?
	if(!m_unstableFluidGroups.empty())
		m_routeCacheVersion++;
	// Apply vision request results.
	for(VisionRequest& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
	m_visionRequestQueue.clear();
	// Do scheduled events.
	executeScheduledEvents(s_step);
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
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
FluidGroup* baseArea::createFluidGroup(const FluidType* fluidType)
{
	m_fluidGroups.emplace_back(fluidType, static_cast<Area*>(this));
	return &m_fluidGroups.back();
}
void baseArea::expireRouteCache(){++m_routeCacheVersion;}
void baseArea::validateAllFluidGroups()
{
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
}
