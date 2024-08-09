#include "hasFluidGroups.h"
#include "../simulation.h"
#include "../area.h"
#include "../fluidType.h"
void AreaHasFluidGroups::doStep()
{
	// Calculate flow.
	std::vector<FluidGroup*> unstable(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	#pragma omp parallel
		for(FluidGroup* group : unstable)
			group->readStep();
	// Remove destroyed.
	for(FluidGroup& fluidGroup : m_fluidGroups)
		if(fluidGroup.m_destroy)
			m_unstableFluidGroups.erase(&fluidGroup);
	std::erase_if(m_fluidGroups, [](FluidGroup& fluidGroup){ return fluidGroup.m_destroy; });
	// Apply flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		fluidGroup->writeStep();
		validateAllFluidGroups();
	}
	// Resolve overfull, diagonal seep, and mist.
	for(FluidGroup* fluidGroup : unstable)
	{
		fluidGroup->afterWriteStep();
		fluidGroup->validate();
	}
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged || fluidGroup->m_disolved; });
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
		[[maybe_unused]] bool found = false;
		for(FluidGroup& fg : m_fluidGroups)
			if(&fg == fluidGroup)
			{
				found = true;
				continue;
			}
		assert(found);
	}
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
}
FluidGroup* AreaHasFluidGroups::createFluidGroup(FluidTypeId fluidType, BlockIndices& blocks, bool checkMerge)
{
	m_fluidGroups.emplace_back(fluidType, blocks, m_area, checkMerge);
	m_unstableFluidGroups.insert(&m_fluidGroups.back());
	//TODO:  If new group is outside register it with areaHasTemperature.
	return &m_fluidGroups.back();
}

void AreaHasFluidGroups::removeFluidGroup(FluidGroup& group)
{
	std::erase_if(m_fluidGroups, [&group](const FluidGroup& g) { return &g == &group; });
}
void AreaHasFluidGroups::clearMergedFluidGroups()
{
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	std::erase_if(m_fluidGroups, [](FluidGroup& fluidGroup){ return fluidGroup.m_merged; });
}
void AreaHasFluidGroups::validateAllFluidGroups()
{
	for(FluidGroup& fluidGroup : m_fluidGroups)
		if(!fluidGroup.m_merged && !fluidGroup.m_destroy)
			fluidGroup.validate();
}
void AreaHasFluidGroups::markUnstable(FluidGroup& fluidGroup)
{
	m_unstableFluidGroups.insert(&fluidGroup);
}
void AreaHasFluidGroups::markStable(FluidGroup& fluidGroup)
{
	m_unstableFluidGroups.erase(&fluidGroup);
}
std::string AreaHasFluidGroups::toS() const
{
	std::string output = std::to_string(m_fluidGroups.size()) + " fluid groups########";
	for(const FluidGroup& fluidGroup : m_fluidGroups)
	{
		output += "type:" + FluidType::getName(fluidGroup.m_fluidType);
		output += "-total:" + std::to_string(fluidGroup.totalVolume().get());
		output += "-blocks:" + std::to_string(fluidGroup.m_drainQueue.m_set.size());
		output += "-status:";
		if(fluidGroup.m_merged)
			output += "-merged";
		if(fluidGroup.m_stable)
			output += "-stable";
		if(fluidGroup.m_disolved)
		{
			output += "-disolved";
			for(const FluidGroup& fg : m_fluidGroups)
				if(fg.m_disolvedInThisGroup.contains(fluidGroup.m_fluidType) && fg.m_disolvedInThisGroup.at(fluidGroup.m_fluidType) == &fluidGroup)
					output += " in " + FluidType::getName(fg.m_fluidType);
		}
		if(fluidGroup.m_destroy)
			output += "-destroy";
		output += "###";
	}
	return output;
}
