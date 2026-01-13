#include "fillQueue.h"
#include "../space/space.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../space/space.h"
#include "numericTypes/types.h"
void FillQueue::initalizeForStep(Area& area, FluidGroup& fluidGroup)
{
	if(!m_set.empty())
	{
		Space& space = area.getSpace();
		RTreeData<CollisionVolume> capacity;
		capacity.insert(m_set, Config::maxPointVolume);
		Density density = FluidType::getDensity(fluidGroup.m_fluidType);
		auto actionSubtract = [&](const Cuboid& cuboid, const FluidData& data) mutable {
			if(FluidType::getDensity(data.type) >= density)
				capacity.updateSubtract(cuboid, data.volume);
		};
		space.fluid_forEachWithCuboid(m_set, actionSubtract);
		m_queue.clear();
		auto actionEmplace = [&](const Cuboid& cuboid, const CollisionVolume& volume)
		{
			for(const Cuboid& flatCuboid : cuboid.sliceAtEachZ())
				m_queue.emplace_back(flatCuboid, volume, CollisionVolume::create(0));
		};
		capacity.queryForEachWithCuboids(space.boundry(), actionEmplace);
		[[maybe_unused]] int32_t totalQueueVolume = 0;
		for(const FutureFlowCuboid& ffc : m_queue)
			totalQueueVolume += ffc.cuboid.volume();
		assert(totalQueueVolume == m_set.volume());
		std::sort(m_queue.begin(), m_queue.end(), compare);
		m_groupStart = m_queue.begin();
		findGroupEnd();
	}
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate();
}
void FillQueue::recordDelta(Area& area, FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep)
{
	assert((m_groupStart->capacity >= volume));
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	assert(flowTillNextStep.exists());
	validate();
	auto& space = area.getSpace();
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
		{
			CuboidSet toAddToFutureNoLongerEmpty = CuboidSet::create(iter->cuboid);
			space.fluid_forEachWithCuboid(toAddToFutureNoLongerEmpty, [&](const Cuboid& cuboid, const FluidData& data) mutable
			{
				if(data.type == fluidGroup.m_fluidType)
					toAddToFutureNoLongerEmpty.maybeRemove(cuboid);
			});
			m_futureNoLongerEmpty.maybeAddAll(toAddToFutureNoLongerEmpty);
		}
		iter->delta += volume;
		assert(iter->delta <= Config::maxPointVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full space and get next group.
	if(flowCapacity == volume)
	{
		assert(!space.fluid_queryAnyWithCondition(m_groupStart->cuboid, [&](const FluidData& data)
		{
			return data.type == fluidGroup.m_fluidType && data.volume + m_groupStart->delta > Config::maxPointVolume;
		}));
		// Record full so it can be removed from fill queue.
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
		{
			if(iter->delta == Config::maxPointVolume)
				m_futureFull.add(iter->cuboid);
			else
			{
				space.fluid_forEachWithCuboid(iter->cuboid, [&](const Cuboid& cuboid, const FluidData& data){
					if(
						data.type == fluidGroup.m_fluidType &&
						data.volume + iter->delta == Config::maxPointVolume
					)
						m_futureFull.add(cuboid.intersection(iter->cuboid));
				});
			}
		}
		m_groupStart = m_groupEnd;
		findGroupEnd();
	}
	// Expand group for new higher level.
	// TODO: continue from current position rather then reseting to start + 1.
	else if(flowTillNextStep == volume)
		findGroupEnd();
	validate();
}
void FillQueue::applyDelta(Area& area, FluidGroup& fluidGroup)
{
	validate();
	Space& space = area.getSpace();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(
				!space.fluid_contains(iter->cuboid, fluidGroup.m_fluidType) ||
				!space.fluid_queryAnyWithCondition(iter->cuboid, [&](const FluidData& data) { return data.type == fluidGroup.m_fluidType && data.group == nullptr; })
		);
		space.fluid_fillInternal(iter->cuboid, iter->delta, fluidGroup);
		/*assert(iter->point->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].second != &m_fluidGroup ||
			(iter->point->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].first < Config::maxPointVolume && !m_futureFull.contains(iter->point)) ||
			(iter->point->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].first == Config::maxPointVolume && m_futureFull.contains(iter->point)));
		*/
		const CuboidSet overfull = space.fluid_queryGetCuboidsOverfull(iter->cuboid).intersection(iter->cuboid);
		if(!overfull.empty())
			m_overfull.addAll(overfull);
	}
	validate();
}
CollisionVolume FillQueue::groupLevel(Area& area, FluidGroup& fluidGroup) const
{
	assert((m_groupStart != m_groupEnd));
	//TODO: calculate this durring find end.
	CollisionVolume highestLevel = CollisionVolume::create(0);
	for(auto it = m_groupStart; it != m_groupEnd; ++it)
	{
		CollisionVolume level = it->delta + area.getSpace().fluid_volumeOfTypeContains(it->cuboid.m_high, fluidGroup.m_fluidType);
		if(level > highestLevel)
			highestLevel = level;

	}
	return highestLevel;
}
void FillQueue::validate() const
{
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	assert((m_groupEnd == m_queue.end() || m_groupEnd == m_groupStart || m_groupStart->cuboid.m_high.z() != m_groupEnd->cuboid.m_high.z() || m_groupStart->capacity > m_groupEnd->capacity));
}
int32_t FillQueue::getPriority(const FutureFlowCuboid& futureFlowPoint)
{
	if(futureFlowPoint.capacity == 0)
		return INT32_MAX;
	//TODO: What is happening here?
	return (( futureFlowPoint.cuboid.m_high.z().get() + 1) * Config::maxPointVolume.get() * 2) - futureFlowPoint.capacity.get();
}
bool FillQueue::compare(const FutureFlowCuboid& a, const FutureFlowCuboid& b)
{
	return getPriority(a) < getPriority(b);
}