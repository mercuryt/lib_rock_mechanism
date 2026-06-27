#include "fluidGroup.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../geometry/cuboidSetHelper.h"
FluidGroup::FluidGroup(const CuboidSet& occupied, int64_t volume, FluidTypeId type, FluidGroupId id) :
	m_occupied(occupied),
	m_volume(volume),
	m_fluidType(type),
	m_id(id)
{
	updateHighAndLowZ();
}
void FluidGroup::maybeDisplaceFromMoreDenseFluid(Area& area)
{
	Space& space = area.getSpace();
	Density density = FluidType::getDensity(m_fluidType);
	SmallSet<FluidGroupId> groupsToDisplaceFrom;
	space.fluid_queryForEach(m_occupied, [&groupsToDisplaceFrom, density](FluidData fluid){
		if(fluid.density > density)
			groupsToDisplaceFrom.maybeInsert(fluid.group);
	});
	CuboidSet displaceFrom;
	for(FluidGroupId groupId : groupsToDisplaceFrom)
	{
		FluidGroup& group = area.m_hasFluidGroups.byId(groupId);
		displaceFrom.maybeAdd(group.intersectionWithFull(m_occupied));
	}
	if(displaceFrom.empty())
		m_flowingUp = false;
	else
	{
		if(displaceFrom.contains(m_occupied))
			m_flowingUp = true;
		else
		{
			m_flowingUp = false;
			m_noLongerOccupied.add(displaceFrom.intersection(m_occupied));
			m_occupied.remove(displaceFrom);
			updateHighAndLowZ();
		}
	}
}
void FluidGroup::maybeExpand(Area& area)
{
	int64_t occupiedVolume = m_occupied.volume();
	m_newlyAdded = m_occupied;
	if(m_volume <= occupiedVolume)
		// Not enough volume to expand horizontally.
		m_newlyAdded.inflateVertical({1});
	else
		m_newlyAdded.inflate({1});
	m_newlyAdded.remove(m_occupied);
	// If the group is overfull expand in all directions, otherwise skip either up or down, depending on direction of flow.
	int64_t maximumFluidVolumeRepresentableByThisPointVolume = occupiedVolume * Config::maxPointVolume.get();
	if(m_volume <= maximumFluidVolumeRepresentableByThisPointVolume)
	{
		Cuboid beyondTrailingLevelQuery = m_flowingUp ?
			Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ.subtractWithMinimum(1)}, Point3D::create(0, 0, m_lowZ.subtractWithMinimum(1).get())) :
			Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ + 1}, Point3D::create(0, 0, m_highZ.get() + 1));
		m_newlyAdded.remove(beyondTrailingLevelQuery);
	}
	Space& space = area.getSpace();
	m_newlyAdded = m_newlyAdded.intersection(space.boundry());
	space.solid_removeAllFrom(m_newlyAdded);
	if(m_newlyAdded.empty())
		// Nowhere to expand to.
		return;
	// Fluids cannot move through higher denstiy fluids when flowing down but can when flowing up.
	if(!m_flowingUp)
		space.fluid_removeAllFilledWithDensityEqualOrGreaterThenFrom(m_newlyAdded, m_fluidType);
	if(m_newlyAdded.empty())
		return;
	// The trailing level is either the top if flowing down or the bottom if flowing up.
	// If drainage is coming from this level only then we need to check there is enough volume in the group to cover more area.
	Cuboid newlyAddedBoundry = m_newlyAdded.boundry();
	bool drainingFromTrailingLevelOnly = false;
	if(m_flowingUp)
	{
		if(newlyAddedBoundry.m_high.z() == m_lowZ)
			drainingFromTrailingLevelOnly = true;
	}
	else
	{
		if(newlyAddedBoundry.m_low.z() == m_highZ)
			drainingFromTrailingLevelOnly = true;
	}
	if(drainingFromTrailingLevelOnly)
	{
		// Draining from the trailing level only.
		Cuboid trailingLevelQuery = m_flowingUp ?
			Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
			Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
		CuboidSet trailingLevel = m_occupied.intersection(trailingLevelQuery);
		int64_t trailingLevelOccupiedVolume = trailingLevel.volume();
		int64_t notTrailingLevelOccupiedVolume = occupiedVolume - trailingLevelOccupiedVolume;
		int64_t notTrailingLevelFluidVolume = std::max(notTrailingLevelOccupiedVolume, notTrailingLevelOccupiedVolume * Config::maxPointVolume.get());
		int64_t trailingLevelFluidVolume = m_volume - notTrailingLevelFluidVolume;
		if(trailingLevelOccupiedVolume + m_newlyAdded.volume() > trailingLevelFluidVolume)
		{
			// Not enough fluid in trailing level to expand.
			m_newlyAdded.clear();
			return;
		}
	}
	m_occupied.add(m_newlyAdded);
	updateHighAndLowZ(m_newlyAdded);
}
void FluidGroup::maybeConsolidate()
{
	if(m_highZ == m_lowZ)
		return;
	int64_t occupiedVolume = m_occupied.volume();
	Cuboid trailingLevelQuery = m_flowingUp ?
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
	CuboidSet trailingLevel = m_occupied.intersection(trailingLevelQuery);
	int64_t trailingLevelOccupiedVolume = trailingLevel.volume();
	int64_t remainderVolume = occupiedVolume - trailingLevelOccupiedVolume;
	int64_t maximumFluidVolumeRepresentableByRemainder = remainderVolume * Config::maxPointVolume.get();
	if(m_volume <= maximumFluidVolumeRepresentableByRemainder)
	{
		// Trailing level can be consolidated without the resulting group being overfull.
		m_occupied.remove(trailingLevelQuery);
		if(m_flowingUp)
			++m_lowZ;
		else
			--m_highZ;
		m_noLongerOccupied.add(trailingLevel);
	}
}
std::vector<std::pair<CuboidSet, int64_t>> FluidGroup::maybeSplit()
{
	if(m_noLongerOccupied.empty())
		return {};
	std::vector<CuboidSet> groups = cuboidSetHelper::splitIntoTouchingGroups(m_occupied);
	if(groups.size() == 1)
		return {};
	// Largest group by size (not volume) will continue to be this one.
	// Size is more useful here because it means less rtree entries to update groupId for.
	auto largest = std::ranges::max_element(groups, {}, [](const CuboidSet& group){ return group.size(); });
	CuboidSet newOccupied = std::move(*largest);
	// Copy volume so we can use it to calculate the new volume while keeping the old volume in place for calculating each group's volume.
	int64_t newVolume = m_volume;
	updateHighAndLowZ();
	(*largest) = std::move(groups.back());
	groups.pop_back();
	// Return other groups to be collected and created after iteration.
	std::vector<std::pair<CuboidSet, int64_t>> output;
	output.reserve(groups.size());
	for(CuboidSet& group : groups)
	{
		m_newlyAdded.maybeRemove(group);
		int64_t volume = getVolume(group);
		output.emplace_back(std::move(group), volume);
		newVolume -= volume;
		// We could add newly split groups to noLongerOccupied but there isn't any reason to.
	}
	m_occupied = std::move(newOccupied);
	m_volume = newVolume;
	return output;
}
void FluidGroup::maybeMerge(Area& area)
{
	if(m_merged || (m_newlyAdded.empty() && !m_checkMergeAll))
		return;
	m_checkMergeAll = false;
	CuboidSet candidates = m_checkMergeAll ? m_occupied : std::move(m_newlyAdded);
	candidates.inflate({1});
	Space& space = area.getSpace();
	SmallSet<FluidGroupId> groupIds;
	groupIds.insert(m_id);
	space.fluid_queryForEach(candidates, [this, &groupIds](FluidData data){
		if(data.type == m_fluidType)
			groupIds.maybeInsert(data.group);
	});
	if(groupIds.size() == 1)
		return;
	std::vector<FluidGroup*> groups;
	groups.reserve(groupIds.size());
	for(FluidGroupId id : groupIds)
		groups.push_back(&area.m_hasFluidGroups.byId(id));
	// Merge into largest by cuboid count rather then by point volume.
	auto largestIter = std::ranges::max_element(groups, {}, [&area](const FluidGroup* group){ return group->m_occupied.size(); });
	FluidGroup& largest = **largestIter;
	(*largestIter) = groups.back();
	groups.pop_back();
	for(FluidGroup* group : groups)
		group->mergeInto(area, largest);
}
void FluidGroup::maybeSetLowerDensityAdjacentUnstable(Area& area)
{
	if(m_noLongerOccupied.empty())
		return;
	Space& space = area.getSpace();
	// Mark fluids with lower density that are adjacent to noLongerOccupied as unstable.
	SmallSet<FluidGroupId> ids;
	Density density = FluidType::getDensity(m_fluidType);
	space.fluid_queryForEach(m_noLongerOccupied.inflated({1}), [&ids, density](FluidData data){
		if(data.density < density)
			ids.maybeInsert(data.group);
	});
	space.fluid_queryForEach(m_newlyAdded, [&ids, density](FluidData data){
		if(data.density < density)
			ids.maybeInsert(data.group);
	});
	for(FluidGroupId id : ids)
		area.m_hasFluidGroups.byId(id).m_stable = false;
}
void FluidGroup::updateHighAndLowZ()
{
	Cuboid boundry = m_occupied.boundry();
	m_highZ = boundry.m_high.z();
	m_lowZ = boundry.m_low.z();
}
void FluidGroup::updateHighAndLowZ(const CuboidSet& added)
{
	for(Cuboid cuboid : added)
	{
		m_highZ = std::max(m_highZ, cuboid.m_high.z());
		m_lowZ = std::min(m_lowZ, cuboid.m_low.z());
	}
}
void FluidGroup::mergeInto(Area& area, FluidGroup& other)
{
	assert(!m_merged);
	assert(m_fluidType == other.m_fluidType);
	m_merged = true;
	other.m_occupied.maybeAdd(m_occupied);
	other.updateHighAndLowZ(m_occupied);
	other.m_volume += m_volume;
	area.getSpace().fluid_setGroupId(m_occupied, m_fluidType, other.m_id);
}
void FluidGroup::addFluid(int64_t quantity)
{
	m_volume += quantity;
}
void FluidGroup::removeFluid(Area& area, int64_t quantity)
{
	assert(m_volume >= quantity);
	if(m_volume == quantity)
	{
		area.m_hasFluidGroups.destroyGroup(m_id);
		area.getSpace().fluid_flowOutFrom(m_occupied, m_fluidType);
	}
	else
	{
		m_volume -= quantity;
		m_stable = false;
	}
}
int64_t FluidGroup::trailingLevelFluidVolume() const
{
	if(m_highZ == m_lowZ)
		return m_volume;
	Cuboid trailingLevelQuery = m_flowingUp ?
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
	CuboidSet trailingLevel = m_occupied.intersection(trailingLevelQuery);
	int64_t trailingLevelOccupiedVolume = trailingLevel.volume();
	int64_t notTrailingLevelOccupiedVolume = m_occupied.volume() - trailingLevelOccupiedVolume;
	int64_t notTrailingLevelFluidVolume = m_volume - (notTrailingLevelOccupiedVolume * Config::maxPointVolume.get());
	return m_volume - notTrailingLevelFluidVolume;
}
int64_t FluidGroup::trailingLevelFluidVolumePerPoint() const
{
	if(m_highZ == m_lowZ)
		return m_volume / m_occupied.volume();
	Cuboid trailingLevelQuery = m_flowingUp ?
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
	CuboidSet trailingLevel = m_occupied.intersection(trailingLevelQuery);
	int64_t trailingLevelOccupiedVolume = trailingLevel.volume();
	int64_t notTrailingLevelOccupiedVolume = m_occupied.volume() - trailingLevelOccupiedVolume;
	int64_t notTrailingLevelFluidVolume = notTrailingLevelOccupiedVolume * Config::maxPointVolume.get();
	int64_t output = (m_volume - notTrailingLevelFluidVolume) / trailingLevelOccupiedVolume;
	assert(output <= Config::maxPointVolume.get() || m_occupied.volume() * Config::maxPointVolume.get() < m_volume);
	return output;
}
int64_t FluidGroup::countPointsOnSurface(Area& area) const
{
	return area.getSpace().m_exposedToSky.get().queryGetIntersection(m_occupied).volume();
}
int64_t FluidGroup::getVolume(const CuboidSet& query) const
{
	if(m_highZ == m_lowZ)
	{
		CuboidSet intersection = query.intersection(m_occupied);
		return (m_volume * intersection.volume() / m_occupied.volume());
	}
	int64_t output{0};
	Cuboid queryBoundry = query.boundry();
	CuboidSet selected = queryBoundry.intersection(m_occupied);
	Cuboid trailingLevelQuery = m_flowingUp ?
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
	CuboidSet trailingLevel = selected.intersection(trailingLevelQuery);
	int64_t trailingLevelOccupiedVolume = trailingLevel.volume();
	int64_t notTrailingLevelOccupiedVolume = selected.volume() - trailingLevelOccupiedVolume;
	output += notTrailingLevelOccupiedVolume * Config::maxPointVolume.get();
	output += trailingLevelOccupiedVolume * trailingLevelFluidVolumePerPoint();
	return output;
}
void FluidGroup::removeFullFrom(CuboidSet& query) const
{
	//TODO:(optimize) Repetitive calculations and excessive copying of cuboids.
	Cuboid queryBoundry = query.boundry();
	Cuboid trailingLevelQuery = m_flowingUp ?
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
	if(!trailingLevelQuery.intersects(queryBoundry) || trailingLevelFluidVolumePerPoint() >= Config::maxPointVolume.get())
		// query only intersects full points.
		query.maybeRemoveAll(m_occupied);
	else
	{
		// Trailing level is not full.
		if(m_highZ == m_lowZ)
			return;
		assert(m_highZ != 0);
		assert(m_lowZ != Distance::max());
		if(!m_flowingUp)
			for(Cuboid cuboid : m_occupied)
			{
				if(cuboid.m_high.z() == m_highZ )
					cuboid.m_high.setZ(m_highZ - 1);
				query.maybeRemove(cuboid);
			}
		else
			for(Cuboid cuboid : m_occupied)
			{
				if(cuboid.m_low.z() == m_lowZ)
					cuboid.m_low.setZ(m_lowZ + 1);
				query.maybeRemove(cuboid);
			}
	}
}
CuboidSet FluidGroup::intersectionWithFull(const CuboidSet& query) const
{
	//TODO:(optimize) Repetitive calculations and excessive coping of cuboids.
	Cuboid queryBoundry = query.boundry();
	Cuboid trailingLevelQuery = m_flowingUp ?
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_lowZ}, Point3D::create(0, 0, m_lowZ.get())) :
		Cuboid::create(Point3D{Distance::max(), Distance::max(), m_highZ}, Point3D::create(0, 0, m_highZ.get()));
	if(!trailingLevelQuery.intersects(queryBoundry) || trailingLevelFluidVolumePerPoint() >= Config::maxPointVolume.get())
		// query only intersects full points.
		return query.intersection(m_occupied);
	else
	{
		// Trailing level is not full.
		if(m_highZ == m_lowZ)
			return {};
		assert(m_highZ != 0);
		assert(m_lowZ != Distance::max());
		CuboidSet output;
		if(!m_flowingUp)
		{
			for(Cuboid cuboid : m_occupied)
			{
				if(cuboid.m_high.z() == m_highZ )
				{
					if(cuboid.sizeZ() == 1)
						continue;
					else
						cuboid.m_high.setZ(m_highZ - 1);
				}
				CuboidSet intersection = query.intersection(cuboid);
				if(intersection.exists())
					output.add(intersection);
			}
		}
		else
		{
			for(Cuboid cuboid : m_occupied)
			{
				if(cuboid.m_low.z() == m_lowZ)
				{
					if(cuboid.sizeZ() == 1)
						continue;
					else
						cuboid.m_low.setZ(m_lowZ + 1);
				}
				CuboidSet intersection = query.intersection(cuboid);
				if(intersection.exists())
					output.add(intersection);
			}
		}
		return output;
	}
	std::unreachable();
}