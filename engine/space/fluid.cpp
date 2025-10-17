#include "space.h"
#include "../fluidType.h"
#include "../mistDisperseEvent.h"
#include "../fluidGroups/fluidGroup.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../deserializationMemo.h"
#include "../numericTypes/types.h"
const FluidData Space::fluid_getData(const Point3D& point, const FluidTypeId& fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	return m_fluid.queryGetOneWithCondition(point, condition);
}
CuboidSet Space::fluid_queryGetCuboids(const Cuboid& shape) const
{
	return m_fluid.queryGetAllCuboids(shape);
}
void Space::fluid_destroyData(const Point3D& point, const FluidTypeId& fluidType)
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	m_fluid.maybeRemoveWithConditionOne(point, condition);
}
void Space::fluid_setTotalVolume(const Point3D& point, const CollisionVolume& newTotal)
{
	m_totalFluidVolume.maybeInsertOrOverwrite(point, newTotal);
}
void Space::fluid_spawnMist(const Point3D& point, const FluidTypeId& fluidType, const Distance maxMistSpread)
{
	const FluidTypeId& mist = m_mist.queryGetOne(point);
	if(mist.exists() && (mist == fluidType || FluidType::getDensity(mist) > FluidType::getDensity(fluidType)))
		return;
	m_mist.maybeInsertOrOverwrite(point, fluidType);
	const Distance inverseDistance = maxMistSpread != 0 ? maxMistSpread : FluidType::getMaxMistSpread(fluidType);
	m_mistInverseDistanceFromSource.maybeInsertOrOverwrite(point, inverseDistance);
	MistDisperseEvent::emplace(m_area, FluidType::getMistDuration(fluidType), fluidType, point);
}
void Space::fluid_clearMist(const Point3D& point)
{
	m_mist.maybeRemove(point);
	m_mistInverseDistanceFromSource.maybeRemove(point);
}
Distance Space::fluid_getMistInverseDistanceToSource(const Point3D& point) const
{
	return m_mistInverseDistanceFromSource.queryGetOne(point);
}
void Space::fluid_mistSetFluidTypeAndInverseDistance(const Point3D& point, const FluidTypeId& fluidType, const Distance& inverseDistance)
{
	m_mist.maybeInsertOrOverwrite(point, fluidType);
	m_mistInverseDistanceFromSource.maybeInsertOrOverwrite(point, inverseDistance);
}
FluidGroup* Space::fluid_getGroup(const Point3D& point, const FluidTypeId& fluidType) const
{
	const FluidData& found = fluid_getData(point, fluidType);
	if(found.empty())
		return nullptr;
	return found.group;
}
void Space::fluid_add(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	for(const Point3D& point : cuboid)
		fluid_add(point, volume, fluidType);
}
void Space::fluid_add(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(!solid_isAny(point));
	// If a suitable fluid group exists at the point already then just add to it.
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	FluidData fluidData = m_fluid.queryGetOneWithCondition(point, condition);
	if(fluidData.exists())
	{
		fluidData.group->addFluid(m_area, volume);
		return;
	}
	auto totalFluidVolume = m_totalFluidVolume.updateAddOne(point, volume);
	bool wasEmpty = totalFluidVolume == volume;
	// Try to find any adjacent groups to join.
	const Cuboid adjacent = getAdjacentWithEdgeAndCornerAdjacent(point);
	FluidData found = m_fluid.queryGetOneWithCondition(adjacent, condition);
	// Fluid Data nullptr will be filled in by either group->addPoint or createFluidGroup.
	m_fluid.maybeInsert(point, {nullptr, fluidType, volume});
	FluidGroup* group = nullptr;
	if(found.empty())
	{
		// No FluidGroup exists at point or adjacent. Create fluid group.
		// Possibly clear space first to prevent reallocation.
		m_area.m_hasFluidGroups.clearMergedFluidGroups();
		group = m_area.m_hasFluidGroups.createFluidGroup(fluidType, {point});
	}
	else
	{
		group = found.group;
		group->addPoint(m_area, point, true);
	}
	assert(group != nullptr);
	m_area.m_hasFluidGroups.markUnstable(*group);
	group->m_stable = false;
	// Shift less dense fluids to excessVolume.
	if(totalFluidVolume > Config::maxPointVolume)
		fluid_resolveOverfull(point);
	m_area.m_hasTerrainFacades.update(adjacent);
	if(!group->m_aboveGround && isExposedToSky(point))
	{
		group->m_aboveGround = true;
		if(FluidType::getFreezesInto(group->m_fluidType).exists())
			m_area.m_hasTemperature.addFreezeableFluidGroupAboveGround(*group);
	}
	floating_maybeFloatUp(point);
	// Float.
	Items& items = m_area.getItems();
	for(const ItemIndex& item : item_getAll(point))
	{
		const Point3D& location = items.getLocation(item);
		if(!items.isFloating(item))
		{
			if(items.canFloatAt(item, location))
				items.setFloating(item, fluidType);
		}
		else
		{
			const Point3D& above = location.above();
			if(items.canFloatAt(item, above))
				items.location_set(item, above, items.getFacing(item));
		}
	}
	if(wasEmpty)
		fluid_maybeRecordFluidOnDeck(point);
}
void Space::fluid_setAllUnstableExcept(const Point3D& point, const FluidTypeId& fluidType)
{
	const auto condition = [&](const FluidData& data) -> bool { return data.type != fluidType; };
	for(const FluidData& data : m_fluid.queryGetAllWithCondition(point, condition))
	{
		data.group->m_stable = false;
		m_area.m_hasFluidGroups.markUnstable(*data.group);
	}
}
void Space::fluid_drainInternal(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	const auto action = [&](FluidData& data){
		if(data.volume == volume)
			data.clear();
		else
			data.volume -= volume;
	};
	const auto condition = [&](const FluidData& data){ return data.type == fluidType; };
	m_fluid.updateOrDestroyActionWithConditionOne(point, action, condition);
	m_totalFluidVolume.updateSubtractOne(point, volume);
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.update(point.getAllAdjacentIncludingOutOfBounds());
	floating_maybeSink(point);
	fluid_maybeEraseFluidOnDeck(point);
}
void Space::fluid_fillInternal(const Point3D& point, const CollisionVolume& volume, FluidGroup& fluidGroup)
{
	bool wasEmpty = false;
	const auto condition = [&](const FluidData& data){ return data.type == fluidGroup.m_fluidType; };
	const auto action = [&](FluidData& data){
		if(data.empty())
		{
			data.group = &fluidGroup;
			data.type = fluidGroup.m_fluidType;
			assert(data.volume == 0);
			wasEmpty = true;
		}
		data.volume += volume;
	};
	m_fluid.updateOrCreateActionWithConditionOne(point, action, condition);
	m_totalFluidVolume.updateAddOne(point, volume);
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.update(point.getAllAdjacentIncludingOutOfBounds());
	floating_maybeFloatUp(point);
	if(wasEmpty)
		fluid_maybeRecordFluidOnDeck(point);
}
void Space::fluid_unsetGroupInternal(const Point3D& point, const FluidTypeId& fluidType)
{
	const auto condition = [&](const FluidData& data){ return data.type == fluidType; };
	const auto action = [](FluidData& data){ data.group = nullptr; };
	m_fluid.updateActionWithConditionOne(point, action, condition);
}
void Space::fluid_setGroupInternal(const Point3D& point, const FluidTypeId& fluidType, FluidGroup& group)
{
	const auto condition = [&](const FluidData& data) { return data.type == fluidType; };
	const auto action = [&](FluidData& data) { data.group = &group; };
	m_fluid.updateActionWithConditionOneAllowNotChanged(point, action, condition);
}
bool Space::fluid_undisolveInternal(const Point3D& point, FluidGroup& fluidGroup)
{
	// Try to find group to add to.
	FluidGroup* groupToAddTo = fluid_getGroup(point, fluidGroup.m_fluidType);
	if(groupToAddTo != nullptr)
	{
		// Group found, merge and mark for destruction.
		groupToAddTo->m_excessVolume += fluidGroup.m_excessVolume;
		fluidGroup.m_destroy = true;
		return true;
	}
	else
	{
		// No group found, try to create one.
		CollisionVolume capacity = fluid_volumeOfTypeCanEnter(point, fluidGroup.m_fluidType);
		if(capacity == 0)
			return false;
		CollisionVolume flow = std::min(capacity, CollisionVolume::create(fluidGroup.m_excessVolume));
		FluidData data = {.group = nullptr, .type = fluidGroup.m_fluidType, .volume = flow};
		m_fluid.maybeInsert(point, data);
		fluidGroup.addPoint(m_area, point, false);
		fluidGroup.m_disolved = false;
		fluidGroup.m_excessVolume -= flow.get();
		m_totalFluidVolume.updateAddOne(point, flow);
		m_area.m_hasTerrainFacades.update(point.getAllAdjacentIncludingOutOfBounds());
		return true;
	}
	return false;
}
void Space::fluid_remove(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(point, fluidType));
	fluid_getGroup(point, fluidType)->removeFluid(m_area, volume);
}
void Space::fluid_removeSyncronus(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(point, fluidType));
	assert(m_totalFluidVolume.queryGetOneOr(point, {0}) >= volume);
	const auto condition = [&](const FluidData& data) { return data.type == fluidType; };
	const auto action = [&](FluidData& data) {
		if(data.volume == volume)
		{
			FluidGroup& group = *data.group;
			if(group.countPoints() == 1)
				m_area.m_hasFluidGroups.removeFluidGroup(group);
			else
				group.removePoint(m_area, point);
			data.clear();
		}
		else
			data.volume -= volume;
	};
	m_fluid.updateOrDestroyActionWithConditionOne(point, action, condition);
	m_totalFluidVolume.updateSubtractOne(point, volume);
	m_area.m_hasTerrainFacades.update(point.getAllAdjacentIncludingOutOfBounds());
	floating_maybeSink(point);
	fluid_maybeEraseFluidOnDeck(point);
}
void Space::fluid_removeAllSyncronus(const Point3D& point)
{
	for(const FluidData& data : m_fluid.queryGetAll(point))
		fluid_removeSyncronus(point, data.volume, data.type);
	assert(!m_totalFluidVolume.queryAny(point));
}
bool Space::fluid_canEnterCurrently(const Point3D& point, const FluidTypeId& fluidType) const
{
	if(m_totalFluidVolume.queryGetOneOr(point, {0}) < Config::maxPointVolume)
		return true;
	for(const FluidData& fluidData : m_fluid.queryGetAll(point))
		if(FluidType::getDensity(fluidData.type) < FluidType::getDensity(fluidType))
			return true;
	return false;
}
bool Space::fluid_canEnterEver(const Point3D& point) const
{
	return !solid_isAny(point);
}
bool Space::fluid_isAdjacentToGroup(const Point3D& point, const FluidGroup& fluidGroup) const
{
	for(const Point3D& adjacent : getDirectlyAdjacent(point))
		if(fluid_contains(adjacent, fluidGroup.m_fluidType) && fluid_getGroup(adjacent, fluidGroup.m_fluidType) == &fluidGroup)
			return true;
	return false;
}
CollisionVolume Space::fluid_volumeOfTypeCanEnter(const Point3D& point, const FluidTypeId& fluidType) const
{
	CollisionVolume output = Config::maxPointVolume;
	for(const FluidData& fluidData : m_fluid.queryGetAll(point))
		if(FluidType::getDensity(fluidData.type) >= FluidType::getDensity(fluidType))
			output -= fluidData.volume;
	return output;
}
CollisionVolume Space::fluid_volumeOfTypeContains(const Point3D& point, const FluidTypeId& fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	const FluidData& data = m_fluid.queryGetOneWithCondition(point, condition);
	// If no data is found with the correct fluid type this will return the default value of 0.
	return data.volume.exists() ? data.volume : CollisionVolume::create(0);
}
FluidTypeId Space::fluid_getTypeWithMostVolume(const Point3D& point) const
{
	assert(m_fluid.queryAny(point));
	CollisionVolume volume = {0};
	FluidTypeId output;
	for(const FluidData& fluidData : m_fluid.queryGetAll(point))
		if(volume < fluidData.volume)
			output = fluidData.type;
	assert(output.exists());
	return output;
}
FluidTypeId Space::fluid_getMist(const Point3D& point) const
{
	return m_mist.queryGetOne(point);
}
void Space::fluid_resolveOverfull(const Point3D& point)
{
	std::vector<FluidTypeId> toErase;
	CollisionVolume totalFluidVolume = m_totalFluidVolume.queryGetOneOr(point, {0});
	assert(totalFluidVolume > Config::maxPointVolume);
	// Iterate fluids starting from the least dense, remove either the total volume of the fluid or the remaining quantity to get the total down to Config::maxPointVolume.
	// The groups' are updated but the fluidData is only a copy.
	auto fluidDataSet = fluid_getAllSortedByDensityAscending(point);
	for(FluidData& fluidData : fluidDataSet)
	{
		assert(fluidData.type == fluidData.group->m_fluidType);
		// Displace lower density fluids.
		CollisionVolume displaced = std::min(fluidData.volume, totalFluidVolume - Config::maxPointVolume);
		assert(displaced != 0);
		fluidData.volume -= displaced;
		totalFluidVolume -= displaced;
		fluidData.group->addFluid(m_area, displaced);
		if(fluidData.volume == 0)
			toErase.push_back(fluidData.type);
		if(fluidData.volume < Config::maxPointVolume)
			fluidData.group->m_fillQueue.maybeAddPoint(point);
		if(totalFluidVolume == Config::maxPointVolume)
			break;
	}
	for(FluidTypeId fluidType : toErase)
	{
		// If the last point of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = fluid_getGroup(point, fluidType);
		assert(fluidGroup->m_fluidType == fluidType);
		fluidGroup->removePoint(m_area, point);
		if(fluidGroup->m_drainQueue.m_set.empty())
		{
			for(FluidData& otherFluidData : fluidDataSet)
				if(FluidType::getDensity(otherFluidData.type) > FluidType::getDensity(fluidType))
				{
					//TODO: find.
					if(otherFluidData.group->m_disolvedInThisGroup.contains(fluidType))
					{
						otherFluidData.group->m_disolvedInThisGroup[fluidType]->m_excessVolume += fluidGroup->m_excessVolume;
						fluidGroup->m_destroy = true;
					}
					else
					{
						otherFluidData.group->m_disolvedInThisGroup.insert(fluidType, fluidGroup);
						fluidGroup->m_disolved = true;
					}
					break;
				}
			assert(fluidGroup->m_disolved || fluidGroup->m_destroy);
		}
		fluid_destroyData(point, fluidType);
	}
	fluid_setTotalVolume(point, totalFluidVolume);
	for(const FluidData& data : fluidDataSet)
	{
		const auto condition = [&](const FluidData& otherData) { return otherData.type == data.type; };
		if(data.volume == 0)
			m_fluid.maybeRemoveWithConditionOne(point, condition);
		else
		{
			const auto action = [&](FluidData& otherData) { otherData.volume = data.volume; };
			m_fluid.updateActionWithConditionOneAllowNotChanged(point, action, condition);
		}
	}
	m_area.m_hasTerrainFacades.update(point.getAllAdjacentIncludingOutOfBounds());
}
void Space::fluid_onPointSetSolid(const Point3D& point)
{
	// Displace fluids.
	bool anyFluidsExist = false;
	SmallSet<FluidData> fluidDataSet;
	m_fluid.maybeUpdateOrDestroyActionAll(point, [&](FluidData& data){
		anyFluidsExist = true;
		data.group->removePoint(m_area, point);
		fluidDataSet.insert(data);
		data.clear();
	});
	if(anyFluidsExist)
		fluid_setTotalVolume(point, {0});
	// Collect adjacent while also removing now solid point from all fill queues of adjacent groups.
	SmallSet<Point3D> candidates;
	SmallSet<Point3D> candidatesAbove;
	for(const Point3D& adjacent : getAdjacentWithEdgeAndCornerAdjacent(point))
		if(fluid_canEnterEver(adjacent))
		{
			if(adjacent.z() == point.z())
				candidates.insert(adjacent);
			else if(adjacent.z() == point.z() + 1)
				candidatesAbove.insert(adjacent);
			auto adjacentFluidDataSet = m_fluid.queryGetAll(adjacent);
			for(FluidData& fluidData : adjacentFluidDataSet)
				fluidData.group->m_fillQueue.maybeRemovePoint(point);
		}
	if(candidates.empty())
		candidates = std::move(candidatesAbove);
	if(candidates.empty())
	{
		// TODO: maybe piston.
		return;
	}
	// Add fluid to adjacent.
	float size = candidates.size();
	SmallMap<FluidTypeId, CollisionVolume> toDispersePerBlock;
	SmallMap<FluidTypeId, CollisionVolume> toDisperseRemainder;
	for(const FluidData& data : fluidDataSet)
	{
		CollisionVolume perBlock = CollisionVolume::create(std::floor((float)data.volume.get() / size));
		CollisionVolume remainder = data.volume - (perBlock * size);
		toDispersePerBlock.insert(data.type, perBlock);
		if(remainder != 0)
			toDisperseRemainder.insert(data.type, remainder);
	}
	for(const Point3D& candidate : candidates)
		for(const auto& [fluidType, volume] : toDispersePerBlock)
			fluid_add({candidate, candidate}, volume, fluidType);
	auto& random = m_area.m_simulation.m_random;
	for(const auto& [fluidType, volume] : toDisperseRemainder)
	{
		Point3D candidate = random.getInVector(candidates.getVector());
		fluid_add({candidate, candidate}, volume, fluidType);
	}
}
void Space::fluid_onPointSetNotSolid(const Point3D& point)
{
	for(const Point3D& adjacent : getDirectlyAdjacent(point))
		if(fluid_canEnterEver(adjacent))
		{
			auto fluidDataSetCopy = m_fluid.queryGetAll(adjacent);
			for(FluidData& fluidData : fluidDataSetCopy)
			{
				fluidData.group->m_fillQueue.maybeAddPoint(point);
				fluidData.group->m_stable = false;
			}
		}
}
CollisionVolume Space::fluid_getTotalVolume(const Point3D& point) const
{
	auto output = m_totalFluidVolume.queryGetOneOr(point, {0});
	CollisionVolume total = {0};
	for(FluidData data : m_fluid.queryGetAll(point))
		total += data.volume;
	assert(total == output);
	return output;
}
const SmallSet<FluidData> Space::fluid_getAllSortedByDensityAscending(const Point3D& point)
{
	auto fluidData = m_fluid.queryGetAll(point);
	std::ranges::sort(fluidData.m_data, std::less<Density>(), [](const FluidData& data) -> Density { return FluidType::getDensity(data.type); });
	return fluidData;
}
const MapWithCuboidKeys<std::pair<FluidTypeId, CollisionVolume>> Space::fluid_getWithCuboidsAndRemoveAll(const CuboidSet& cuboids)
{
	MapWithCuboidKeys<std::pair<FluidTypeId, CollisionVolume>> output;
	for(const Cuboid& cuboid : cuboids)
		for(const auto& [fluidCuboid, fluidData] : m_fluid.queryGetAllWithCuboids(cuboid))
			output.insert({fluidCuboid, {fluidData.type, fluidData.volume}});
	for(const Cuboid& cuboid : cuboids)
		m_fluid.removeAll(cuboid);
	return output;
}
bool Space::fluid_any(const Point3D& point) const
{
	return m_fluid.queryAny(point);
}
template<typename ShapeT>
bool Space::fluid_contains(const ShapeT& shape, const FluidTypeId& fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	return m_fluid.queryAnyWithCondition(shape, condition);
}
template bool Space::fluid_contains(const Point3D& shape, const FluidTypeId& fluidType) const;
template bool Space::fluid_contains(const Cuboid& shape, const FluidTypeId& fluidType) const;
template<typename ShapeT>
Point3D Space::fluid_containsPoint(const ShapeT& shape, const FluidTypeId& fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	const Cuboid result = m_fluid.queryGetOneCuboidWithCondition(shape, condition);
	if(result.empty())
		return Point3D::null();
	return result.intersection(shape).m_high;

}
template Point3D Space::fluid_containsPoint(const Point3D& shape, const FluidTypeId& fluidType) const;
template Point3D Space::fluid_containsPoint(const Cuboid& shape, const FluidTypeId& fluidType) const;
void Space::fluid_maybeRecordFluidOnDeck(const Point3D& point)
{
	assert(m_totalFluidVolume.queryAny(point));
	const DeckId& deckId = m_area.m_decks.queryDeckId(point);
	if(deckId.exists())
	{
		const ActorOrItemIndex& isOnDeckOf = m_area.m_decks.getForId(deckId);
		assert(isOnDeckOf.isItem());
		const ItemIndex& item = isOnDeckOf.getItem();
		m_area.getItems().onDeck_recordPointContainingFluid(item, point);
	}
}
void Space::fluid_maybeEraseFluidOnDeck(const Point3D& point)
{
	const DeckId& deckId = m_area.m_decks.queryDeckId(point);
	if(deckId.exists())
	{
		const ActorOrItemIndex& isOnDeckOf = m_area.m_decks.getForId(deckId);
		assert(isOnDeckOf.isItem());
		const ItemIndex& item = isOnDeckOf.getItem();
		m_area.getItems().onDeck_erasePointContainingFluid(item, point);
	}
}