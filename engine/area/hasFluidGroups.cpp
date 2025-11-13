#include "hasFluidGroups.h"
#include "../simulation/simulation.h"
#include "../area/area.h"
#include "../fluidType.h"
void AreaHasFluidGroups::doStep(bool parallel)
{
	if(m_unstableFluidGroups.empty())
		// All groups are stable, nothing to do here.
		return;
	// Calculate flow.
	// unstable will be used to iterate the contents of m_unstableFluidGroups while altering it. It will be reinitalized with fresh copies several times.
	SmallSet<FluidGroup*> unstable = m_unstableFluidGroups;
	#ifndef NDEBUG
		for (const FluidGroup *group : unstable)
			assert(!group->m_stable);
	#endif
	if(parallel)
	{
		// OpenMP doesn't work with SmallSet?
		std::vector<FluidGroup*>& groups = unstable.getVector();
		//#pragma omp parallel for
			for(FluidGroup* group : groups)
				group->readStep(m_area);
	}
	else
		for(FluidGroup* group : unstable)
			group->readStep(m_area);
	// Remove destroyed.
	m_unstableFluidGroups.eraseIf([](auto* fluidGroup){ return fluidGroup->m_destroy; });
	std::erase_if(m_fluidGroups, [](const FluidGroup& fluidGroup){ return fluidGroup.m_destroy; });
	// Apply flow.
	validateAllFluidGroups();
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		fluidGroup->writeStep(m_area);
		validateAllFluidGroups();
	}
	// Resolve overfull, diagonal seep, and mist.
	unstable = m_unstableFluidGroups;
	for(FluidGroup* fluidGroup : unstable)
	{
		fluidGroup->afterWriteStep(m_area);
		fluidGroup->validate(m_area);
	}
	m_unstableFluidGroups.eraseIf([](const FluidGroup* fluidGroup){ return fluidGroup->m_merged || fluidGroup->m_disolved; });
	// Split.
	// Reinitalize unstable with filtered set again.
	unstable = m_unstableFluidGroups;
	for(FluidGroup* fluidGroup : unstable)
		fluidGroup->splitStep(m_area);
	// Merge.
	// Reinitalize unstable with filtered set.
	unstable = m_unstableFluidGroups;
	for(FluidGroup* fluidGroup : unstable)
		fluidGroup->mergeStep(m_area);
	m_unstableFluidGroups.eraseIf([](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(m_area);
	// Check for groups to remove from unstable, possibly gather them in toErase.
	SmallSet<FluidGroup*> toErase;
	for(FluidGroup& fluidGroup : m_fluidGroups)
	{
		if(fluidGroup.m_excessVolume <= 0 && fluidGroup.m_drainQueue.m_set.size() == 0)
		{
			fluidGroup.m_destroy = true;
			assert(!fluidGroup.m_disolved);
		}
		if(fluidGroup.m_destroy || fluidGroup.m_merged || fluidGroup.m_disolved || fluidGroup.m_stable)
			m_unstableFluidGroups.maybeErase(&fluidGroup);
		else if(!fluidGroup.m_stable)
			m_unstableFluidGroups.maybeInsert(&fluidGroup);
		if(fluidGroup.m_destroy || fluidGroup.m_merged)
		{
			toErase.insert(&fluidGroup);
			if(fluidGroup.m_destroy)
				assert(fluidGroup.m_drainQueue.m_set.empty());
		}
	}
	// Validate in the context of toErase.
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(m_area, toErase);
	// Destroy groups in toErase.
	m_fluidGroups.remove_if([&](FluidGroup& fluidGroup){ return toErase.contains(&fluidGroup); });
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(m_area);
	// Validate that all groups in m_unstable are still in m_fluidGroups.
	for(const FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		[[maybe_unused]] bool found = false;
		assert(!fluidGroup->m_stable);
		for(FluidGroup& fg : m_fluidGroups)
			if(&fg == fluidGroup)
			{
				found = true;
				continue;
			}
		assert(found);
	}
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(m_area);
}
FluidGroup* AreaHasFluidGroups::createFluidGroup(const FluidTypeId& fluidType, const CuboidSet& space, bool checkMerge)
{
	FluidGroup& fluidGroup = m_fluidGroups.emplace_back(m_allocator, fluidType, space, m_area, checkMerge);
	assert(!fluidGroup.m_stable);
	return &fluidGroup;
}
FluidGroup* AreaHasFluidGroups::createFluidGroup(const FluidTypeId& fluidType, CuboidSet&& space, bool checkMerge)
{
	return createFluidGroup(fluidType, space, checkMerge);
}
void AreaHasFluidGroups::removeFluidGroup(FluidGroup& group)
{
	if(group.m_aboveGround)
		// Use maybe remove because even if the group is marked as above ground it might be wrong: it is permitted to be incorrect becaues it will be checked when the group is about to freeze.
		m_area.m_hasTemperature.maybeRemoveFreezeableFluidGroupAboveGround(group);
	std::erase(m_fluidGroups, group);
}
void AreaHasFluidGroups::clearMergedFluidGroups()
{
	m_unstableFluidGroups.eraseIf([](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	std::erase_if(m_fluidGroups, [](FluidGroup& fluidGroup){ return fluidGroup.m_merged; });
}
void AreaHasFluidGroups::validateAllFluidGroups()
{
	for(FluidGroup& fluidGroup : m_fluidGroups)
		if(!fluidGroup.m_merged && !fluidGroup.m_destroy)
			fluidGroup.validate(m_area);
}
void AreaHasFluidGroups::markUnstable(FluidGroup& fluidGroup)
{
	m_unstableFluidGroups.maybeInsert(&fluidGroup);
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
		output += "type:" + (FluidType::getName(fluidGroup.m_fluidType));
		output += "-total:" + std::to_string(fluidGroup.totalVolume(m_area).get());
		output += "-space:" + std::to_string(fluidGroup.m_drainQueue.m_set.size());
		output += "-status:";
		if(fluidGroup.m_merged)
			output += "-merged";
		if(fluidGroup.m_stable)
			output += "-stable";
		if(fluidGroup.m_disolved)
		{
			output += "-disolved";
			for(const FluidGroup& fg : m_fluidGroups)
				if(fg.m_disolvedInThisGroup.contains(fluidGroup.m_fluidType) && fg.m_disolvedInThisGroup[fluidGroup.m_fluidType] == &fluidGroup)
					output += " in " + (FluidType::getName(fg.m_fluidType));
		}
		if(fluidGroup.m_destroy)
			output += "-destroy";
		output += "###";
	}
	return output;
}
bool AreaHasFluidGroups::contains(const FluidGroup& fluidGroup)
{
	return std::ranges::find(m_fluidGroups, fluidGroup) != m_fluidGroups.end();
}
