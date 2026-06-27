#include "areaHasFluidGroups.h"
#include "../area/area.h"
#include "../space/space.h"
AreaHasFluidGroups::AreaHasFluidGroups(Area& area) :
	m_area(area)
{ }
void AreaHasFluidGroups::doStep()
{
	// Collect groups created by split in newGroups so they can be instanced after iteration completes.
	SmallMap<FluidTypeId, std::vector<std::pair<CuboidSet, int64_t>>> newGroups;
	Space& space = m_area.getSpace();
	for(FluidGroup& group : m_groups)
	{
		if(group.m_stable)
			continue;
		// Prepare.
		group.m_occupied.prepare();
		// Flow.
		group.maybeDisplaceFromMoreDenseFluid(m_area);
		group.maybeExpand(m_area);
		group.maybeConsolidate();
		if(group.m_newlyAdded.empty() && group.m_noLongerOccupied.empty())
			group.m_stable = true;
		else
		{
			space.fluid_flowInto(group.m_newlyAdded, group.m_fluidType, group);
			space.fluid_flowOutFrom(group.m_noLongerOccupied, group.m_fluidType);
			group.maybeSetLowerDensityAdjacentUnstable(m_area);
		}
		// Find new groups to split.
		std::vector<std::pair<CuboidSet, int64_t>> newGroupsFromThisGroup = group.maybeSplit();
		if(!newGroupsFromThisGroup.empty())
		{
			std::vector<std::pair<CuboidSet, int64_t>>& groupsForFluidType = newGroups.getOrCreate(group.m_fluidType);
			for(auto& [cuboidSet, volume] : newGroupsFromThisGroup)
				groupsForFluidType.emplace_back(std::move(cuboidSet), volume);
		}
	}
	// Create newly split off groups.
	for(auto [fluidType, groups] : newGroups)
		for(auto [occupied, volume] : groups)
		{
			createGroup(occupied, volume, fluidType);
			space.fluid_setGroupId(occupied, fluidType, m_nextId - 1);
		}
	// Merge.
	// Since the group constructor sets newlyAdded equal to occupied this will merge newly split groups.
	for(FluidGroup& group : m_groups)
		group.maybeMerge(m_area);
	clearMerged();
	// Reset.
	for(FluidGroup& group : m_groups)
	{
		group.m_newlyAdded.clear();
		group.m_noLongerOccupied.clear();
	}
}
void AreaHasFluidGroups::createGroup(const CuboidSet& occupied, int64_t volume, FluidTypeId type)
{
	m_groups.emplace_back(occupied, volume, type, m_nextId++);
}
void AreaHasFluidGroups::destroyGroup(FluidGroupId id)
{
	auto iter = std::ranges::find(m_groups, id, &FluidGroup::m_id);
	(*iter) = std::move(m_groups.back());
	m_groups.pop_back();
}
void AreaHasFluidGroups::clearMerged()
{
	std::erase_if(m_groups, [](const FluidGroup& group){ return group.m_merged; });
}
FluidGroup& AreaHasFluidGroups::byId(FluidGroupId id)
{
	auto found = std::ranges::find(m_groups, id, &FluidGroup::m_id);
	assert(found != m_groups.end());
	return *found;
}
bool AreaHasFluidGroups::hasUnstable() const
{
	return std::ranges::find(m_groups, false, &FluidGroup::m_stable) != m_groups.end();
}
void to_json(Json& data, const AreaHasFluidGroups& groups)
{
	data = Json::array();
	for(const FluidGroup& group : groups.m_groups)
		data.push_back({
			{"type", group.m_fluidType},
			{"volume", group.m_volume},
			{"cuboids", group.m_occupied}
		});
}
void from_json(const Json& data, AreaHasFluidGroups& groups)
{
	Space& space = groups.m_area.getSpace();
	for(const Json& groupData : data)
		space.fluid_add(
			groupData["cuboids"].get<CuboidSet>(),
			groupData["volume"].get<int64_t>(),
			groupData["type"].get<FluidTypeId>()
		);
}