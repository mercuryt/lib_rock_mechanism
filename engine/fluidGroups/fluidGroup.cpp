/*
 * A set of space which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_pointsByZandTotalFluidHeight is all space which have fluid. It is sorted by high to low. This is the source of flow.
 * 	m_adjacentAndUnfullPointsByInverseZandTotalFluidHeight is all space which have some fluid but aren't full, as well as space with no fluid that fluid can enter and are adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#include "fluidGroup.h"
#include "../space/space.h"
#include "../fluidType.h"
#include "../numericTypes/types.h"
#include "../util.h"
#include "../config.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../dataStructures/smallSet.h"

#include <queue>
#include <cassert>
#include <numeric>

//TODO: reuse space as m_fillQueue.m_set.
FluidGroup::FluidGroup(Allocator& allocator, const FluidTypeId& ft, SmallSet<Point3D>& space, Area& area, bool checkMerge) :
	m_fillQueue(allocator), m_drainQueue(allocator), m_fluidType(ft)
{
	for(const Point3D& point : space)
		if( area.getSpace().fluid_contains(point, m_fluidType))
			addPoint(area, point, checkMerge);
}
void FluidGroup::addFluid(Area& area, const CollisionVolume& volume)
{
	m_excessVolume += volume.get();
	setUnstable(area);
}
void FluidGroup::removeFluid(Area& area, const CollisionVolume& volume)
{
	m_excessVolume -= volume.get();
	setUnstable(area);
}
void FluidGroup::addPoint(Area& area, const Point3D& point, bool checkMerge)
{
	assert(!m_merged);
	assert( area.getSpace().fluid_contains(point, m_fluidType));
	// Already recorded this one.
	if(m_drainQueue.m_set.contains(point))
		return;
	setUnstable(area);
	// Cannot be in fill queue if full. Must be otherwise.
	Space& space = area.getSpace();
	const FluidData* found = space.fluid_getData(point, m_fluidType);
	if(found && found->volume < Config::maxPointVolume)
		m_fillQueue.maybeAddPoint(point);
	else
		m_fillQueue.maybeRemovePoint(point);
	// Set group membership in Space.
	found = area.getSpace().fluid_getData(point, m_fluidType);
	FluidGroup* oldGroup = found->group;
	if(oldGroup != nullptr && oldGroup != this)
		oldGroup->removePoint(area, point);
	space.fluid_setGroupInternal(point, m_fluidType, *this);
	m_drainQueue.maybeAddPoint(point);
	// Add adjacent if fluid can enter.
	SmallSet<FluidGroup*> toMerge;
	for(const Point3D& adjacent : area.getSpace().getDirectlyAdjacent(point))
	{
		if(!area.getSpace().fluid_canEnterEver(adjacent))
			continue;
		const FluidData* found = space.fluid_getData(adjacent, m_fluidType);
		// Merge groups if needed.
		if(found && checkMerge)
		{
			assert(!found->group->m_merged);
			if(found->group == this)
				continue;
			toMerge.maybeInsert(found->group);
		}
		if(!found || found->volume < Config::maxPointVolume)
			m_fillQueue.maybeAddPoint(adjacent);
	}
	// TODO: (performance) collect all groups to merge recursively and then merge all into the largest.
	FluidGroup* larger = this;
	for(FluidGroup* oldGroup : toMerge)
		// oldGroup may have been merged by recursion in a previous iteration.
		if(!oldGroup->m_merged)
			larger = larger->merge(area, oldGroup);
	larger->addMistFor(area, point);
}
void FluidGroup::removePoint(Area& area, const Point3D& point)
{
	setUnstable(area);
	m_drainQueue.removePoint(point);
	m_potentiallyNoLongerAdjacentFromSyncronusStep.insert(point);
	for(const Point3D& adjacent : area.getSpace().getDirectlyAdjacent(point))
		if( area.getSpace().fluid_canEnterEver(adjacent))
		{
			//Check for group split.
			auto found = area.getSpace().fluid_getData(adjacent, m_fluidType);
			if(found && found->group == this)
				m_potentiallySplitFromSyncronusStep.maybeInsert(adjacent);
			else
				//Check for empty adjacent to remove.
				m_potentiallyNoLongerAdjacentFromSyncronusStep.maybeInsert(adjacent);
		}
}
void FluidGroup::setUnstable(Area& area)
{
	m_stable = false;
	area.m_hasFluidGroups.markUnstable(*this);
}
void FluidGroup::addMistFor(Area& area, const Point3D& point)
{
	auto& space = area.getSpace();
	if(FluidType::getMistDuration(m_fluidType) != 0 &&
		(
			point.z() == 0 ||
			!space.solid_is(point.below())
		)
	)
		for(const Point3D& adjacent : space.getAdjacentOnSameZLevelOnly(point))
			if(space.fluid_canEnterEver(adjacent))
				space.fluid_spawnMist(adjacent, m_fluidType);
}
FluidGroup* FluidGroup::merge(Area& area, FluidGroup* smaller)
{
	assert(smaller != this);
	assert(smaller->m_fluidType == m_fluidType);
	assert(!smaller->m_merged);
	assert(!m_merged);
	assert(!smaller->m_disolved);
	assert(!m_disolved);
	assert(!smaller->m_destroy);
	assert(!m_destroy);

	auto& space = area.getSpace();
	FluidGroup* larger = this;
	if(smaller->m_drainQueue.m_set.size() > larger->m_drainQueue.m_set.size())
		std::swap(smaller, larger);
	// Mark as merged rather then destroying right away so as not to interfear with iteration.
	smaller->m_merged = true;
	larger->setUnstable(area);
	// Add excess volume.
	larger->m_excessVolume += smaller->m_excessVolume;
	// Merge queues.
	larger->m_drainQueue.merge(smaller->m_drainQueue);
	larger->m_fillQueue.merge(smaller->m_fillQueue);
	// Set fluidGroup for merged space.
	for(const Point3D& point : smaller->m_drainQueue.m_set)
	{
		assert(space.fluid_contains(point, m_fluidType));
		space.fluid_setGroupInternal(point, m_fluidType, *larger);
		assert(larger->m_drainQueue.m_set.contains(point));
	}
	// Merge other fluid groups ment to merge with smaller with larger instead.
	for(const Point3D& point : smaller->m_futureNewEmptyAdjacents)
	{
		auto found = space.fluid_getData(point, m_fluidType);
		if(!found)
			continue;
		if(!found->group->m_merged && found->group != larger)
		{
			larger->merge(area, found->group);
			if(larger->m_merged)
				larger = found->group;
		}
	}
	return larger;
}
void FluidGroup::readStep(Area& area)
{
	assert(!m_merged);
	assert(!m_destroy);
	assert(!m_stable);
	assert(!m_disolved);
	auto& space = area.getSpace();
	m_futureNewEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futureNotifyPotentialUnfullAdjacent.clear();
	m_viscosity = FluidType::getViscosity(m_fluidType);
	m_drainQueue.initalizeForStep(area, *this);
	m_fillQueue.initalizeForStep(area, *this);
	validate(area);
	// If there is no where to flow into there is nothing to do.
	if(m_fillQueue.m_set.empty() ||
		((m_fillQueue.groupSize() == 0 || m_fillQueue.groupCapacityPerPoint() == 0) && m_excessVolume >= 0 ) ||
		((m_drainQueue.groupSize() == 0 || m_drainQueue.groupCapacityPerPoint() == 0) && m_excessVolume <= 0)
	)
	{
		m_stable = true;
		m_fillQueue.noChange();
		m_drainQueue.noChange();
		return;
	}
	// Disperse m_excessVolume.
	while(m_excessVolume > 0 && m_fillQueue.groupSize() != 0 && m_fillQueue.m_groupStart != m_fillQueue.m_queue.end())
	{
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)m_excessVolume < m_fillQueue.groupSize())
			break;
		// How much fluid is there space for total.
		CollisionVolume flowCapacity = m_fillQueue.groupCapacityPerPoint();
		// How much can be filled before the next low point(s).
		CollisionVolume flowTillNextStep = m_fillQueue.groupFlowTillNextStepPerPoint();
		if(flowTillNextStep.empty())
			// If unset then set to max in order to exclude from std::min
			flowTillNextStep = CollisionVolume::max();
		CollisionVolume excessPerPoint = CollisionVolume::create(m_excessVolume / m_fillQueue.groupSize());
		CollisionVolume flowPerPoint = std::min({flowTillNextStep, flowCapacity, excessPerPoint});
		m_excessVolume -= flowPerPoint.get() * m_fillQueue.groupSize();
		m_fillQueue.recordDelta(area, *this, flowPerPoint, flowCapacity, flowTillNextStep);
	}
	while(m_excessVolume < 0 && m_drainQueue.groupSize() != 0 && m_drainQueue.m_groupStart != m_drainQueue.m_queue.end())
	{
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)(-1 * m_excessVolume) < m_drainQueue.groupSize())
			break;
		// How much is avaliable to drain total.
		CollisionVolume flowCapacity = m_drainQueue.groupCapacityPerPoint();
		// How much can be drained before the next high point(s).
		CollisionVolume flowTillNextStep = m_drainQueue.groupFlowTillNextStepPerPoint();
		if(flowTillNextStep.empty())
			// If unset then set to max in order to exclude from std::min
			flowTillNextStep = CollisionVolume::max();
		CollisionVolume excessPerPoint = CollisionVolume::create((-1 * m_excessVolume) / m_drainQueue.groupSize());
		CollisionVolume flowPerPoint = std::min({flowCapacity, flowTillNextStep, excessPerPoint});
		m_excessVolume += (flowPerPoint * m_drainQueue.groupSize()).get();
		m_drainQueue.recordDelta(area, flowPerPoint, flowCapacity, flowTillNextStep);
	}
	// Do primary flow.
	// If we have reached the end of either queue the loop ends.
	while(m_viscosity > 0 && m_drainQueue.groupSize() != 0 && m_fillQueue.groupSize() != 0)
	{
		assert(m_fillQueue.m_groupEnd == m_fillQueue.m_queue.end() ||
				m_fillQueue.m_groupStart->point.z() != m_fillQueue.m_groupEnd->point.z() ||
				m_fillQueue.m_groupStart->capacity != m_fillQueue.m_groupEnd->capacity);
		CollisionVolume drainVolume = m_drainQueue.groupLevel(area, *this);
		assert(drainVolume != 0);
		CollisionVolume fillVolume = m_fillQueue.groupLevel(area, *this);
		assert(fillVolume < Config::maxPointVolume);
		//uint32_t fillInverseCapacity = Config::maxPointVolume - m_fillQueue.m_groupStart->capacity;
		//assert(m_drainQueue.m_groupStart->point->m_z > m_fillQueue.m_groupStart->point->m_z || drainVolume >= fillVolume);
		//uint32_t drainZ = m_drainQueue.m_groupStart->point->m_z;
		//uint32_t fillZ = m_fillQueue.m_groupStart->point->m_z;
		//TODO: using fillVolume > drainVolume here rather then the above assert feels like a hack.
		//if(drainZ < fillZ || (drainZ == fillZ && (fillVolume >= drainVolume || (drainVolume == 1 && fillVolume == 0))))
		[[maybe_unused]] bool stopHere = area.m_simulation.m_step == 6 && fillVolume == 0 && drainVolume == 2;
		if(dispositionIsStable(fillVolume, drainVolume))
		{
			// if no new space have been added this step then set stable
			if(m_fillQueue.m_futureNoLongerEmpty.empty() && m_disolvedInThisGroup.empty())
			{
				m_stable = true;
			}
			break;
		}
		for(auto itd = m_drainQueue.m_groupStart; itd != m_drainQueue.m_groupEnd; ++itd)
			if(itd->delta != 0)
				for(auto itf = m_fillQueue.m_groupStart; itf != m_fillQueue.m_groupEnd; ++itf)
					if(itf->delta != 0)
						assert(itf->point != itd->point);
		for(auto itf = m_fillQueue.m_groupStart; itf != m_fillQueue.m_groupEnd; ++itf)
			if(itf->delta != 0)
				for(auto itd = m_drainQueue.m_groupStart; itd != m_drainQueue.m_groupEnd; ++itd)
					if(itd->delta != 0)
						assert(itf->point != itd->point);
		// How much fluid is there space for total.
		CollisionVolume flowCapacityFill = m_fillQueue.groupCapacityPerPoint();
		// How much can be filled before the next low point(s).
		CollisionVolume flowTillNextStepFill = m_fillQueue.groupFlowTillNextStepPerPoint();
		if(flowTillNextStepFill.empty())
			// If the next step is on another z level then this value will be empty. Set it to max to exclude it from the std::min call later.
			flowTillNextStepFill = CollisionVolume::max();
		// How much is avaliable to drain total.
		CollisionVolume flowCapacityDrain = m_drainQueue.groupCapacityPerPoint();
		// How much can be drained before the next high point(s).
		CollisionVolume flowTillNextStepDrain = m_drainQueue.groupFlowTillNextStepPerPoint();
		if(flowTillNextStepDrain.empty())
			// If the next step is on another z level then this value will be empty. Set it to max to exclude it from the std::min call later.
			flowTillNextStepDrain = CollisionVolume::max();
		// How much can drain before equalization. Only applies if the groups are on the same z.
		CollisionVolume maxDrainForEqualibrium, maxFillForEquilibrium;
		if(m_fillQueue.m_groupStart->point.z() ==m_drainQueue.m_groupStart->point.z())
		{
			CollisionVolume totalLevel = (fillVolume * m_fillQueue.groupSize()) + (drainVolume * m_drainQueue.groupSize());
			uint32_t totalCount = m_fillQueue.groupSize() + m_drainQueue.groupSize();
			// We want to round down here so default truncation is fine.
			CollisionVolume equalibriumLevel = totalLevel / totalCount;
			maxDrainForEqualibrium = drainVolume - equalibriumLevel;
			maxFillForEquilibrium = equalibriumLevel - fillVolume;
		}
		else
			maxDrainForEqualibrium = maxFillForEquilibrium = CollisionVolume::max();
		// Viscosity is applied only to fill.
		// Fill is allowed to be 0. If there is not enough volume in drain group to put at least one in each of fill group then we let all of drain group go to excess volume.
		CollisionVolume PerPointFill = std::min({flowCapacityFill, flowTillNextStepFill, maxFillForEquilibrium, CollisionVolume::create(m_viscosity)});
		CollisionVolume PerPointDrain = std::min({flowCapacityDrain, flowTillNextStepDrain, maxDrainForEqualibrium});
		assert(PerPointDrain != 0);
		CollisionVolume totalFill = PerPointFill * m_fillQueue.groupSize();
		CollisionVolume totalDrain = PerPointDrain * m_drainQueue.groupSize();
		if(totalFill < totalDrain)
		{
			if(maxFillForEquilibrium == PerPointFill)
				PerPointDrain = maxDrainForEqualibrium;
			else
				PerPointDrain = CollisionVolume::create(std::ceil((float)totalFill.get() / (float)m_drainQueue.groupSize()));
			totalDrain = PerPointDrain * m_drainQueue.groupSize();
		}
		else if(totalFill > totalDrain)
		{
			// If we are steping into equilibrium.
			if(maxDrainForEqualibrium == PerPointDrain)
				PerPointFill = maxFillForEquilibrium;
			else
				// truncate to round down
				PerPointFill = totalDrain / (float)m_fillQueue.groupSize();
			totalFill = PerPointFill * m_fillQueue.groupSize();
		}
		assert(totalFill <= totalDrain);
		// Viscosity is consumed by flow.
		m_viscosity -= PerPointFill.get();
		// Record changes.
		m_drainQueue.recordDelta(area, PerPointDrain, flowCapacityDrain, flowTillNextStepDrain);
		if(PerPointFill != 0)
			m_fillQueue.recordDelta(area, *this, PerPointFill, flowCapacityFill, flowTillNextStepFill);
		m_excessVolume += (int)totalDrain.get() - (int)totalFill.get();
		// If we are at equilibrium then stop looping.
		// Don't mark stable because there may be newly added adjacent to flow into next tick.
		if(PerPointDrain == maxDrainForEqualibrium)
			break;
	}
	// Flow loops completed, analyze results.
	SmallSet<Point3D> futurePoints;
	//futurePoints.reserve(m_drainQueue.m_set.size() + m_fillQueue.m_futureNoLongerEmpty.size());
	for(const Point3D& point : m_drainQueue.m_set)
		if(!m_drainQueue.m_futureEmpty.contains(point))
			futurePoints.insert(point);
	futurePoints.maybeInsertAll(m_fillQueue.m_futureNoLongerEmpty);
	if(futurePoints.empty())
	{
		assert(m_excessVolume < 1);
		m_destroy = true;
	}
	// We can't return here because we need to convert descriptive future into proscriptive future.
	// Find future space for futureEmptyAdjacent.
	for(const Point3D& point : m_fillQueue.m_futureNoLongerEmpty)
	{
		for(const Point3D& adjacent : space.getDirectlyAdjacent(point))
			if( area.getSpace().fluid_canEnterEver(adjacent) && !m_drainQueue.m_set.contains(adjacent) && !m_fillQueue.m_set.contains(adjacent)
			 )
			{
				auto found = area.getSpace().fluid_getData(adjacent, m_fluidType);
				if(!found || found->volume < Config::maxPointVolume || found->group != this)
					m_futureNewEmptyAdjacents.maybeInsert(adjacent);
			}
	}
	// -Find any potental newly created groups.
	// Collect space adjacent to newly empty which are !empty.
	// Also collect possiblyNoLongerAdjacent.
	SmallSet<Point3D> potentialNewGroups;
	potentialNewGroups.swap(m_potentiallySplitFromSyncronusStep);
	SmallSet<Point3D> possiblyNoLongerAdjacent;
	possiblyNoLongerAdjacent.swap(m_potentiallyNoLongerAdjacentFromSyncronusStep);
	// Collect all adjacent to futureEmpty which fluid can enter ever.
	SmallSet<Point3D> adjacentToFutureEmpty;
	//adjacentToFutureEmpty.reserve(m_drainQueue.m_futureEmpty.size() * 6);
	for(const Point3D& point : m_drainQueue.m_futureEmpty)
	{
		for(const Point3D& adjacent : space.getDirectlyAdjacent(point))
			if( area.getSpace().fluid_canEnterEver(adjacent))
				adjacentToFutureEmpty.maybeInsert(adjacent);
	}
	for(const Point3D& point : adjacentToFutureEmpty)
		// If point won't be empty then check for forming a new group as it may be detached.
		if(futurePoints.contains(point))
			potentialNewGroups.maybeInsert(point);
		// Else check for removal from empty adjacent queue.
		else
			possiblyNoLongerAdjacent.maybeInsert(point);
	// Seperate into contiguous groups. Each point in potentialNewGroups might be in a seperate group.
	// If there is only one potential new group there can not be a split: there needs to be another group to split from.
	potentialNewGroups.eraseIf([&](const Point3D& point){ return !futurePoints.contains(point); });
	if(potentialNewGroups.size() > 1)
	{
		SmallSet<Point3D> closed;
		//closed.reserve(futurePoints.size());
		for(const Point3D& point : potentialNewGroups)
		{
			if(closed.contains(point))
				continue;
			auto condition = [&](const Point3D& point){ return futurePoints.contains(point); };
			SmallSet<Point3D> adjacents = space.collectAdjacentsWithCondition(point, condition);
			// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
			closed.maybeInsertAll(adjacents);
			m_futureGroups.emplace_back(adjacents);
		}
	}
	if(!m_futureGroups.empty())
	{
		// Find the largest.
		auto condition = [](FluidGroupSplitData& a, FluidGroupSplitData& b){ return a.members.size() < b.members.size(); };
		auto it = std::ranges::max_element(m_futureGroups, condition);
		// Put it at the end because it needs to be in a known place and it will be removed later.
		std::rotate(it, it + 1, m_futureGroups.end());
		// Record each group's future new adjacent.
		for(const Point3D& point : m_futureNewEmptyAdjacents)
			for(const Point3D& adjacent : space.getDirectlyAdjacent(point))
				if( area.getSpace().fluid_canEnterEver(adjacent))
				{
					for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
						if(fluidGroupSplitData.members.contains(adjacent))
							fluidGroupSplitData.futureAdjacent.maybeInsert(point);
				}
	}
	// -Find no longer adjacent empty to remove from fill queue and newEmptyAdjacent.
	SmallSet<Point3D> futureRemoveFromEmptyAdjacents;
	//futureRemoveFromEmptyAdjacents.reserve(possiblyNoLongerAdjacent.size());
	for(const Point3D& point : possiblyNoLongerAdjacent)
	{
		if(futurePoints.contains(point))
			continue;
		bool stillAdjacent = false;
		for(const Point3D& adjacent : space.getDirectlyAdjacent(point))
			if( area.getSpace().fluid_canEnterEver(adjacent) && futurePoints.contains(adjacent))
			{
				stillAdjacent = true;
				break;
			}
		if(!stillAdjacent)
			futureRemoveFromEmptyAdjacents.insert(point);
	}
	// Convert descriptive future to proscriptive future.
	m_futureAddToDrainQueue = m_fillQueue.m_futureNoLongerEmpty;
	m_futureRemoveFromDrainQueue = m_drainQueue.m_futureEmpty;
	//m_futureAddToFillQueue = (m_futureNoLongerFull - futureRemoveFromEmptyAdjacents) + m_futureNewEmptyAdjacents;
	//m_futureAddToFillQueue.reserve(m_futureNewEmptyAdjacents.size() + m_drainQueue.m_futureNoLongerFull.size());
	m_futureAddToFillQueue = m_futureNewEmptyAdjacents;
	for(const Point3D& point : m_drainQueue.m_futureNoLongerFull)
		if(!futureRemoveFromEmptyAdjacents.contains(point))
			m_futureAddToFillQueue.insert(point);
	//m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents + m_futureFull;
	//m_futureRemoveFromFillQueue.reserve(futureRemoveFromEmptyAdjacents.size() + m_fillQueue.m_futureFull.size());
	m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents;
	m_futureRemoveFromFillQueue.maybeInsertAll(m_fillQueue.m_futureFull);
	validate(area);
}
void FluidGroup::writeStep(Area& area)
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	validate(area);
	auto& space = area.getSpace();
	area.m_hasFluidGroups.validateAllFluidGroups();
	m_drainQueue.applyDelta(area, *this);
	m_fillQueue.applyDelta(area, *this);
	// Update queues.
	validate(area);
	for([[maybe_unused]] const Point3D& point : m_futureAddToFillQueue)
		assert(!m_futureRemoveFromFillQueue.contains(point));
	// Don't add to drain queue if taken by another fluid group already.
	m_futureAddToDrainQueue.eraseIf([&](const Point3D& point){
		auto found = space.fluid_getData(point, m_fluidType);
		return found && found->group != this;
	});
	for([[maybe_unused]] const Point3D& point : m_futureAddToDrainQueue)
		assert(!m_futureRemoveFromDrainQueue.contains(point));
	for(const Point3D& point : m_futureRemoveFromFillQueue)
	{
		bool tests = (
			space.fluid_contains(point, m_fluidType) ||
			space.fluid_volumeOfTypeContains(point, m_fluidType) == Config::maxPointVolume ||
			space.fluid_getGroup(point, m_fluidType) != this
		);
		assert(tests || !m_futureGroups.empty());
		if(!tests && !m_futureGroups.empty())
		{
			[[maybe_unused]] bool found = false;
			for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
				if(fluidGroupSplitData.members.contains(point))
				{
					found = true;
					break;
				}
			assert(found);
		}
	}
	for(const Point3D& point : m_futureRemoveFromDrainQueue)
	{
		bool tests = (
			!space.fluid_contains(point, m_fluidType) ||
			space.fluid_getGroup(point, m_fluidType) != this
		);
		assert(tests || !m_futureGroups.empty());
		if(!tests && !m_futureGroups.empty())
		{
			[[maybe_unused]] bool found = false;
			for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
				if(fluidGroupSplitData.members.contains(point))
				{
					found = true;
					break;
				}
			assert(found);
		}
	}
	m_fillQueue.removePoints(m_futureRemoveFromFillQueue);
	m_fillQueue.maybeAddPoints(m_futureAddToFillQueue);
	m_drainQueue.removePoints(m_futureRemoveFromDrainQueue);
	m_drainQueue.maybeAddPoints(m_futureAddToDrainQueue);
	validate(area);
	area.m_hasFluidGroups.validateAllFluidGroups();
}
void FluidGroup::afterWriteStep(Area& area)
{
	// Any fluid group could be marked as disolved or destroyed during iteration.
	if(m_disolved || m_destroy)
		return;
	validate(area);
	assert(!m_merged);
	auto& space = area.getSpace();
	// Do seeping through corners if enabled.
	// Resolve overfull space.
	validate(area);
	for(const Point3D& point : m_fillQueue.m_overfull)
		if(space.fluid_getTotalVolume(point) > Config::maxPointVolume)
			space.fluid_resolveOverfull(point);
	validate(area);
	for(const Point3D& point : m_fillQueue.m_futureNoLongerEmpty)
		addMistFor(area, point);
	validate(area);
}
void FluidGroup::splitStep(Area& area)
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	validate(area);
	// Disperse disolved, split off fluidGroups of another fluidType.
	// TODO: Transfer ownership to adjacent fluid groups with a lower density then this one but higher then disolved group.
	auto& space = area.getSpace();
	std::vector<FluidTypeId> dispersed;
	for(auto& [fluidType, disolvedFluidGroup] : m_disolvedInThisGroup)
	{
		assert(disolvedFluidGroup->m_drainQueue.m_set.empty());
		assert(m_fluidType != fluidType);
		assert(disolvedFluidGroup->m_fluidType == fluidType);
		assert(disolvedFluidGroup->m_excessVolume > 0);
		for(FutureFlowPoint& futureFlowPoint : m_fillQueue.m_queue)
			if(space.fluid_undisolveInternal(futureFlowPoint.point, *disolvedFluidGroup))
			{
				dispersed.push_back(fluidType);
				break;
			}
	}
	validate(area);
	for(FluidTypeId fluidType : dispersed)
		m_disolvedInThisGroup.erase(fluidType);
	// Split off future groups of this fluidType.
	if(m_futureGroups.empty() || m_futureGroups.size() == 1)
		return;
	SmallSet<Point3D> formerMembers;
	for(const Point3D& point : m_drainQueue.m_set)
		if(!m_futureGroups.back().members.contains(point))
		{
			formerMembers.insert(point);
			// Unset recorded group from space which are about to be split off so the fluidGroup constructor does not try to remove them again.
			space.fluid_unsetGroupInternal(point, m_fluidType);
		}
	m_drainQueue.removePoints(formerMembers);
	SmallSet<Point3D> formerFill;
	for(const Point3D& point : m_fillQueue.m_set)
		if(!m_drainQueue.m_set.contains(point) && !point.isAdjacentToAny(m_drainQueue.m_set))
			formerFill.insert(point);
	m_fillQueue.removePoints(formerFill);
	m_futureNewEmptyAdjacents = m_futureGroups.back().futureAdjacent;
	// Remove largest group, it will remain as this instance.
	m_futureGroups.pop_back();
	// Create new groups.
	for(auto& [members, newAdjacent] : m_futureGroups)
	{
		FluidGroup* fluidGroup = area.m_hasFluidGroups.createFluidGroup(m_fluidType, members, false);
		fluidGroup->m_futureNewEmptyAdjacents.swap(newAdjacent);
	}
	validate(area);
}
// Do all split prior to doing merge.
void FluidGroup::mergeStep(Area& area)
{
	assert(!m_disolved);
	assert(!m_destroy);
	validate(area);
	auto& space = area.getSpace();
	// Record merge. First group consumes subsequent groups.
	for(const Point3D& point : m_futureNewEmptyAdjacents)
	{
		// Fluid groups may be marked merged during merge iteration.
		if(m_merged)
			return;
		auto found = space.fluid_getData(point, m_fluidType);
		if(!found)
			continue;
		found->group->validate(area);
		assert(!found->group->m_merged);
		assert(found->group->m_fluidType == m_fluidType);
		if(found->group != this)
		{
			merge(area, found->group);
			continue;
		}
	}
	validate(area);
}
CollisionVolume FluidGroup::totalVolume(Area& area) const
{
	CollisionVolume output = CollisionVolume::create(m_excessVolume);
	for(const Point3D& point : m_drainQueue.m_set)
		output += area.getSpace().fluid_volumeOfTypeContains(point, m_fluidType);
	return output;
}
bool FluidGroup::dispositionIsStable(const CollisionVolume& fillVolume, const CollisionVolume& drainVolume) const
{
	Distance drainZ = m_drainQueue.m_groupStart->point.z();
	Distance fillZ = m_fillQueue.m_groupStart->point.z();
	if(drainZ < fillZ)
		return true;
	if(drainZ == fillZ &&
		(
			fillVolume >= drainVolume ||
			(
				fillVolume == 0 &&
				(
					drainVolume == 1 ||
					drainVolume * m_drainQueue.groupSize() < (m_fillQueue.groupSize() + m_drainQueue.groupSize())
				)
			)
		)
	)
		return true;
	return false;
}
Quantity FluidGroup::countPointsOnSurface(const Area& area) const
{
	const Space& space = area.getSpace();
	// TODO: could be optimized by only looking at the topmost z level in DrainQueue::m_queue
	return Quantity::create(m_drainQueue.m_set.countIf([&](const Point3D& point){ return space.isExposedToSky(point); }));
}
void FluidGroup::validate(Area& area) const
{
	assert(area.m_hasFluidGroups.contains(*this));
	if(m_merged || m_destroy || m_disolved)
		return;
	for(const Point3D& point : m_drainQueue.m_set)
	{
		assert(point.exists());
		for(const FluidData& fluidData : area.getSpace().fluid_getAll(point))
		{
			if(fluidData.group == this)
				continue;
			assert(fluidData.group->m_fluidType == fluidData.type);
			assert(fluidData.group->m_drainQueue.m_set.contains(point));
		}
	}
}
void FluidGroup::validate(Area& area, [[maybe_unused]] SmallSet<FluidGroup*> toErase) const
{
	if(m_merged || m_destroy || m_disolved)
		return;
	for(const Point3D& point : m_drainQueue.m_set)
		for(const FluidData& fluidData : area.getSpace().fluid_getAll(point))
		{
			if(fluidData.group == this)
				continue;
			assert(fluidData.group->m_fluidType == fluidData.type);
			assert(fluidData.group->m_drainQueue.m_set.contains(point));
			assert(!toErase.contains(fluidData.group));
		}
}
void FluidGroup::log(Area& area) const
{
	Space& space = area.getSpace();
	for(const FutureFlowPoint& futureFlowPoint : m_drainQueue.m_queue)
	{
		if(&*m_drainQueue.m_groupEnd == &futureFlowPoint)
			std::cout << "drain group end\n";
		futureFlowPoint.point.log();
		std::cout <<
			"d:" << futureFlowPoint.delta.get() << " " <<
			"c:" << futureFlowPoint.capacity.get() << " " <<
			"v:" << space.fluid_volumeOfTypeContains(futureFlowPoint.point, m_fluidType).get() << " " <<
			"t:" << space.fluid_getTotalVolume(futureFlowPoint.point).get();
		std::cout << '\n';
	}
	std::cout << "total:" << totalVolume(area).get() << '\n';
	if(m_excessVolume)
		std::cout << "excess:" << m_excessVolume << '\n';
	if(m_merged)
		std::cout << "merged" << '\n';
	if(m_destroy)
		std::cout << "destroy" << '\n';
	if(m_disolved)
		std::cout << "disolved" << '\n';
	if(m_stable)
		std::cout << "stable" << '\n';
	std::cout << std::endl;
}
void FluidGroup::logFill(Area& area) const
{
	Space& space = area.getSpace();
	for(const FutureFlowPoint& futureFlowPoint : m_fillQueue.m_queue)
	{
		if(&*m_fillQueue.m_groupEnd == &futureFlowPoint)
			std::cout << "fill group end\n";
		futureFlowPoint.point.log();
		std::cout <<
			"d:" << futureFlowPoint.delta.get() << " " <<
			"c:" << futureFlowPoint.capacity.get() << " " <<
			"v:" << space.fluid_volumeOfTypeContains(futureFlowPoint.point, m_fluidType).get() << " " <<
			"t:" << space.fluid_getTotalVolume(futureFlowPoint.point).get();
		std::cout << std::endl;
	}
}