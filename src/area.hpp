/*
* A game map.
*/

#pragma once
#include "area.h"
#include "block.hpp"
#include <algorithm>

template<class Block, class Actor, class DerivedArea, class FluidType>
BaseArea<Block, Actor, DerivedArea, FluidType>::BaseArea(uint32_t x, uint32_t y, uint32_t z) :
	m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_locationBuckets(static_cast<DerivedArea&>(*this)), m_routeCacheVersion(0), m_visionCuboidsActive(false), m_destroy(false)
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
				m_blocks[x][y][z].setup(static_cast<DerivedArea*>(this), x, y, z);
		}
	}
	// record adjacent blocks
	for(uint32_t x = 0; x < m_sizeX; ++x)
		for(uint32_t y = 0; y < m_sizeY; ++y)
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				m_blocks[x][y][z].recordAdjacent();
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::readStep()
{ 
	//TODO: Count tasks dispatched and finished instead of pool.wait_for_tasks so we can do multiple areas simultaniously in one pool.
	// Process vision, generate and push_task requests for every actor in current bucket.
	// It seems like having the vision requests permanantly embeded in the actors and iterating the vision bucket directly rather then using the visionRequestQueue should be faster but limited testing shows otherwise.
	m_blocksWithChangedTemperature.clear();
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(s_step))
		m_visionRequestQueue.emplace_back(*actor);
	auto visionIter = m_visionRequestQueue.begin();
	while(visionIter < m_visionRequestQueue.end())
	{
		auto end = std::min(m_visionRequestQueue.end(), visionIter + Config::visionThreadingBatchSize);
		s_pool.push_task([=](){ VisionRequest<Block, Actor, DerivedArea>::readSteps(visionIter, end); });
		visionIter = end;
	}
	// Calculate cave in.
	s_pool.push_task([&](){ stepCaveInRead(); });
	// Calculate flow.
	for(FluidGroup<Block, DerivedArea, FluidType>* fluidGroup : m_unstableFluidGroups)
		s_pool.push_task([=](){ fluidGroup->readStep(); });
	// Calculate routes.
	auto routeIter = m_routeRequestQueue.begin();
	while(routeIter < m_routeRequestQueue.end())
	{
		auto end = std::min(m_routeRequestQueue.end(), routeIter + Config::routeThreadingBatchSize);
		s_pool.push_task([=](){ RouteRequest<Block, Actor, DerivedArea>::readSteps(routeIter, end); });
		routeIter = end;
	}
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::writeStep()
{ 
	s_pool.wait_for_tasks();
	// Apply flow.
	for(FluidGroup<Block, DerivedArea, FluidType>* fluidGroup : m_unstableFluidGroups)
	{
		fluidGroup->writeStep();
		validateAllFluidGroups();
	}
	// Resolve overfull, diagonal seep, and mist.
	// Make vector of unstable so we can iterate it while modifing the original.
	std::vector<FluidGroup<Block, DerivedArea, FluidType>*> unstable(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup<Block, DerivedArea, FluidType>* fluidGroup : unstable)
	{
		fluidGroup->afterWriteStep();
		fluidGroup->validate();
	}
	std::erase_if(m_unstableFluidGroups, [](FluidGroup<Block, DerivedArea, FluidType>* fluidGroup){ return fluidGroup->m_stable || fluidGroup->m_disolved || fluidGroup->m_destroy || fluidGroup->m_merged; });
	std::vector<FluidGroup<Block, DerivedArea, FluidType>*> unstable2(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup<Block, DerivedArea, FluidType>* fluidGroup : unstable2)
		fluidGroup->mergeStep();
	std::erase_if(m_unstableFluidGroups, [](FluidGroup<Block, DerivedArea, FluidType>* fluidGroup){ return fluidGroup->m_merged; });
	// Apply fluid split.
	std::vector<FluidGroup<Block, DerivedArea, FluidType>*> unstable3(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup<Block, DerivedArea, FluidType>* fluidGroup : unstable3)
		fluidGroup->splitStep();
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	std::unordered_set<FluidGroup<Block, DerivedArea, FluidType>*> toErase;
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
	{
		if(fluidGroup.m_excessVolume <= 0 && fluidGroup.m_drainQueue.m_set.size() == 0)
		{
			fluidGroup.m_destroy = true;
			assert(!fluidGroup.m_disolved);
		}
		if(fluidGroup.m_destroy || fluidGroup.m_merged || fluidGroup.m_disolved || fluidGroup.m_stable)
			m_unstableFluidGroups.erase(&fluidGroup);
		else if(!fluidGroup.m_stable) // This seems avoidable.
			m_unstableFluidGroups.insert(&fluidGroup);
		if(fluidGroup.m_destroy || fluidGroup.m_merged)
		{
			toErase.insert(&fluidGroup);
			if(fluidGroup.m_destroy)
				assert(fluidGroup.m_drainQueue.m_set.empty());
		}
	}
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
		fluidGroup.validate(toErase);
	m_fluidGroups.remove_if([&](FluidGroup<Block, DerivedArea, FluidType>& fluidGroup){ return toErase.contains(&fluidGroup); });
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	for(const FluidGroup<Block, DerivedArea, FluidType>* fluidGroup : m_unstableFluidGroups)
	{
		bool found = false;
		for(FluidGroup<Block, DerivedArea, FluidType>& fg : m_fluidGroups)
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
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// Apply routes.
	for(RouteRequest<Block, Actor, DerivedArea>& routeRequest : m_routeRequestQueue)
		routeRequest.writeStep();
	m_routeRequestQueue.clear();
	// If there is any unstable groups expire route caches.
	// TODO: Be more selective?
	if(!m_unstableFluidGroups.empty())
		m_routeCacheVersion++;
	// Apply vision request results.
	for(VisionRequest<Block, Actor, DerivedArea>& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
	m_visionRequestQueue.clear();
	// Do scheduled events.
	m_eventSchedule.execute(s_step);
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// Clean up old vision cuboids.
	if(m_visionCuboidsActive)
		std::erase_if(m_visionCuboids, [](VisionCuboid<Block, Actor, DerivedArea>& visionCuboid){ return visionCuboid.m_destroy; });
	// Apply temperature changes.
	for(auto& [block, oldDelta] : m_blocksWithChangedTemperature)
	{
		uint32_t ambientTemperature = block->getAmbientTemperature();
		block->applyTemperatureChange(ambientTemperature + oldDelta, ambientTemperature + block->m_deltaTemperature);
	}
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::registerActor(Actor& actor)
{
	m_visionBuckets.add(actor);
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::unregisterActor(Actor& actor)
{
	m_visionBuckets.remove(actor);
	if(actor.m_taskEvent != nullptr)
		actor.m_taskEvent->cancel();
	if(actor.m_location != nullptr)
		actor.m_location->m_area->m_locationBuckets.erase(actor);
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::scheduleMove(Actor& actor)
{
	Block* block = *(actor.m_routeIter);
	uint32_t stepsToMove = block->moveCost(actor.m_moveType, actor.m_location) / actor.getSpeed();
	MoveEvent<Block, Actor>::emplace(m_eventSchedule, stepsToMove, actor);
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::registerRouteRequest(Actor& actor, bool detour)
{
	m_routeRequestQueue.emplace_back(actor, detour);
}
template<class Block, class Actor, class DerivedArea, class FluidType>
FluidGroup<Block, DerivedArea, FluidType>* BaseArea<Block, Actor, DerivedArea, FluidType>::createFluidGroup(const FluidType* fluidType, std::unordered_set<Block*>& blocks, bool checkMerge)
{
	m_fluidGroups.emplace_back(fluidType, blocks, static_cast<DerivedArea&>(*this), checkMerge);
	m_unstableFluidGroups.insert(&m_fluidGroups.back());
	return &m_fluidGroups.back();
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::visionCuboidsActivate()
{
	m_visionCuboidsActive = true;
	VisionCuboid<Block, Actor, DerivedArea>::setup(static_cast<DerivedArea&>(*this));
}
template<class Block, class Actor, class DerivedArea, class FluidType>
BaseCuboid<Block, Actor, DerivedArea> BaseArea<Block, Actor, DerivedArea, FluidType>::getZLevel(uint32_t z)
{
	return BaseCuboid<Block, Actor, DerivedArea>(m_blocks[m_sizeX - 1][m_sizeY - 1][z], m_blocks[0][0][z]);
}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::expireRouteCache(){++m_routeCacheVersion;}
template<class Block, class Actor, class DerivedArea, class FluidType>
void BaseArea<Block, Actor, DerivedArea, FluidType>::validateAllFluidGroups()
{
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
		if(!fluidGroup.m_merged && !fluidGroup.m_destroy)
			fluidGroup.validate();
}
template<class Block, class Actor, class DerivedArea, class FluidType>
std::string BaseArea<Block, Actor, DerivedArea, FluidType>::toS()
{
	std::string output = std::to_string(m_fluidGroups.size()) + " fluid groups########";
	for(FluidGroup<Block, DerivedArea, FluidType>& fluidGroup : m_fluidGroups)
	{
		output += "type:" + fluidGroup.m_fluidType->name;
		output += "-total:" + std::to_string(fluidGroup.totalVolume());
		output += "-blocks:" + std::to_string(fluidGroup.m_drainQueue.m_set.size());
		output += "-status:";
		if(fluidGroup.m_merged)
			output += "-merged";
		if(fluidGroup.m_stable)
			output += "-stable";
		if(fluidGroup.m_disolved)
		{
			output += "-disolved";
			for(FluidGroup<Block, DerivedArea, FluidType>& fg : m_fluidGroups)
				if(fg.m_disolvedInThisGroup.contains(fluidGroup.m_fluidType) && fg.m_disolvedInThisGroup.at(fluidGroup.m_fluidType) == &fluidGroup)
					output += " in " + fg.m_fluidType->name;
		}
		if(fluidGroup.m_destroy)
			output += "-destroy";
		output += "###";
	}
	return output;
}
template<class Block, class Actor, class DerivedArea, class FluidType>
BaseArea<Block, Actor, DerivedArea, FluidType>::~BaseArea()
{
	m_destroy = true;
}
// Include implimentations of classes that Area depends on here so it doesn't have to be replicated by the user.
#include "visionRequest.hpp"
#include "routeRequest.hpp"
#include "fluidQueue.hpp"
#include "fillQueue.hpp"
#include "drainQueue.hpp"
#include "fluidGroup.hpp"
#include "caveIn.hpp"
#include "moveEvent.hpp"
#include "visionCuboid.hpp"
#include "cuboid.hpp"
#include "locationBuckets.hpp"
