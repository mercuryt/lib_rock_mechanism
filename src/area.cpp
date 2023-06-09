/*
* A game map.
*/

#include "area.h"
#include "simulation.h"
#include <algorithm>

Area::Area(uint32_t x, uint32_t y, uint32_t z) :
	m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_actorLocationBuckets(*this), m_visionCuboidsActive(false), m_areaHasTemperature(*this)
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
				m_blocks[x][y][z].setup(*this, x, y, z);
		}
	}
	// record adjacent blocks
	for(uint32_t x = 0; x < m_sizeX; ++x)
		for(uint32_t y = 0; y < m_sizeY; ++y)
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				m_blocks[x][y][z].recordAdjacent();
}
void Area::readStep(BS::thread_pool_light pool)
{ 
	//TODO: Count tasks dispatched and finished instead of pool.wait_for_tasks so we can do multiple areas simultaniously in one pool.
	// Process vision, emplace requests for every actor in current bucket.
	// It seems like having the vision requests permanantly embeded in the actors and iterating the vision bucket directly rather then using the visionRequestQueue should be faster but limited testing shows otherwise.
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(simulation::step))
		m_visionRequestQueue.emplace_back(*actor);
	auto visionIter = m_visionRequestQueue.begin();
	while(visionIter < m_visionRequestQueue.end())
	{
		auto end = std::min(m_visionRequestQueue.end(), visionIter + Config::visionThreadingBatchSize);
		pool.push_task([=](){ VisionRequest::readSteps(visionIter, end); });
		visionIter = end;
	}
	// Calculate cave in.
	pool.push_task([&](){ stepCaveInRead(); });
	// Calculate flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
		pool.push_task([=](){ fluidGroup->readStep(); });
}
void Area::writeStep()
{ 
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
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	std::vector<FluidGroup*> unstable2(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable2)
		fluidGroup->mergeStep();
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	// Apply fluid split.
	std::vector<FluidGroup*> unstable3(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable3)
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
		else if(!fluidGroup.m_stable) // This seems avoidable.
			m_unstableFluidGroups.insert(&fluidGroup);
		if(fluidGroup.m_destroy || fluidGroup.m_merged)
		{
			toErase.insert(&fluidGroup);
			if(fluidGroup.m_destroy)
				assert(fluidGroup.m_drainQueue.m_set.empty());
		}
	}
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(toErase);
	m_fluidGroups.remove_if([&](FluidGroup& fluidGroup){ return toErase.contains(&fluidGroup); });
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
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
	// Apply cave in.
	if(!m_caveInData.empty())
		stepCaveInWrite();
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// If there is any unstable groups expire route caches.
	// TODO: Be more selective?
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// Clean up old vision cuboids.
	if(m_visionCuboidsActive)
		std::erase_if(m_visionCuboids, [](VisionCuboid& visionCuboid){ return visionCuboid.m_destroy; });
}
void Area::registerActor(Actor& actor)
{
	m_visionBuckets.add(actor);
}
void Area::unregisterActor(Actor& actor)
{
	m_visionBuckets.remove(actor);
	if(actor.m_location != nullptr)
		actor.m_location->m_area->m_actorLocationBuckets.erase(actor);
}
FluidGroup* Area::createFluidGroup(const FluidType& fluidType, std::unordered_set<Block*>& blocks, bool checkMerge)
{
	m_fluidGroups.emplace_back(fluidType, blocks, *this, checkMerge);
	m_unstableFluidGroups.insert(&m_fluidGroups.back());
	return &m_fluidGroups.back();
}
void Area::visionCuboidsActivate()
{
	m_visionCuboidsActive = true;
	VisionCuboid::setup(*this);
}
Cuboid Area::getZLevel(uint32_t z)
{
	return Cuboid(m_blocks[m_sizeX - 1][m_sizeY - 1][z], m_blocks[0][0][z]);
}
void Area::validateAllFluidGroups()
{
	for(FluidGroup& fluidGroup : m_fluidGroups)
		if(!fluidGroup.m_merged && !fluidGroup.m_destroy)
			fluidGroup.validate();
}
std::string Area::toS()
{
	std::string output = std::to_string(m_fluidGroups.size()) + " fluid groups########";
	for(FluidGroup& fluidGroup : m_fluidGroups)
	{
		output += "type:" + fluidGroup.m_fluidType.name;
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
			for(FluidGroup& fg : m_fluidGroups)
				if(fg.m_disolvedInThisGroup.contains(&fluidGroup.m_fluidType) && fg.m_disolvedInThisGroup.at(&fluidGroup.m_fluidType) == &fluidGroup)
					output += " in " + fg.m_fluidType.name;
		}
		if(fluidGroup.m_destroy)
			output += "-destroy";
		output += "###";
	}
	return output;
}
