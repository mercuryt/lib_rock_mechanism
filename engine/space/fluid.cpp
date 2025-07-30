#include "space.h"
#include "../fluidType.h"
#include "../mistDisperseEvent.h"
#include "../fluidGroups/fluidGroup.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../deserializationMemo.h"
#include "../numericTypes/types.h"
const FluidData* Space::fluid_getData(const Point3D& point, const FluidTypeId& fluidType) const
{
	const auto& fluid = m_fluid.queryGetOne(point);
	auto iter = std::ranges::find(fluid, fluidType, &FluidData::type);
	if(iter == fluid.end())
		return nullptr;
	return &*iter;
}
void Space::fluid_destroyData(const Point3D& point, const FluidTypeId& fluidType)
{
	const std::vector<FluidData>& fluid = m_fluid.queryGetOne(point);
	auto iter = std::ranges::find(fluid, fluidType, &FluidData::type);
	assert(iter != fluid.end());
	if(fluid.size() == 1)
		m_fluid.remove(point);
	else
	{
		uint offset = iter - fluid.begin();
		auto fluidCopy = fluid;
		std::swap(fluidCopy[offset], fluidCopy.back());
		fluidCopy.pop_back();
		m_fluid.insert(point, std::move(fluidCopy));
	}
}
void Space::fluid_setTotalVolume(const Point3D& point, const CollisionVolume& newTotal)
{
	[[maybe_unused]] CollisionVolume checkTotal = {0};
	for(const FluidData& data : m_fluid.queryGetOne(point))
		checkTotal += data.volume;
	assert(checkTotal == newTotal);
	m_totalFluidVolume.maybeInsert(point, newTotal);
}
void Space::fluid_spawnMist(const Point3D& point, const FluidTypeId& fluidType, const Distance maxMistSpread)
{
	const FluidTypeId& mist = m_mist.queryGetOne(point);
	if(mist.exists() && (mist == fluidType || FluidType::getDensity(mist) > FluidType::getDensity(fluidType)))
		return;
	m_mist.maybeInsert(point, fluidType);
	const Distance inverseDistance = maxMistSpread != 0 ? maxMistSpread : FluidType::getMaxMistSpread(fluidType);
	m_mistInverseDistanceFromSource.maybeInsert(point, inverseDistance);
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
	m_mist.maybeInsert(point, fluidType);
	m_mistInverseDistanceFromSource.maybeInsert(point, inverseDistance);
}
FluidGroup* Space::fluid_getGroup(const Point3D& point, const FluidTypeId& fluidType) const
{
	auto found = fluid_getData(point, fluidType);
	if(!found)
		return nullptr;
	assert(found->type == fluidType);
	return found->group;
}
void Space::fluid_add(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	for(const Point3D& point : cuboid)
		fluid_add(point, volume, fluidType);
}
void Space::fluid_add(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(!solid_is(point));
	bool wasEmpty = m_totalFluidVolume.queryGetOne(point) == 0;
	// If a suitable fluid group exists already then just add to it's excessVolume.
	auto found = fluid_getData(point, fluidType);
	if(found)
	{
		assert(found->type == fluidType);
		found->group->addFluid(m_area, volume);
		return;
	}
	const auto& fluidData = m_fluid.queryGetOne(point);
	auto fluidDataCopy = fluidData;
	fluidDataCopy.emplace_back(fluidType, nullptr, volume);
	if(!fluidData.empty())
		m_fluid.remove(point);
	m_fluid.insert(point, std::move(fluidDataCopy));
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(const Point3D& adjacent : getDirectlyAdjacent(point))
		if(fluid_canEnterEver(adjacent) && fluid_contains(adjacent, fluidType))
		{
			fluidGroup = fluid_getGroup(adjacent, fluidType);
			fluidGroup->addPoint(m_area, point, true);
			break;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
		fluidGroup = m_area.m_hasFluidGroups.createFluidGroup(fluidType, {point});
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
	const auto& totalFluidVolume = m_totalFluidVolume.queryGetOneOr(point, {0});
	fluid_setTotalVolume(point, totalFluidVolume + volume);
	// Shift less dense fluids to excessVolume.
	if(totalFluidVolume > Config::maxPointVolume)
		fluid_resolveOverfull(point);
	m_area.m_hasTerrainFacades.update({point, point});
	assert(fluidGroup != nullptr);
	if(!fluidGroup->m_aboveGround && isExposedToSky(point))
	{
		fluidGroup->m_aboveGround = true;
		if(FluidType::getFreezesInto(fluidGroup->m_fluidType).exists())
			m_area.m_hasTemperature.addFreezeableFluidGroupAboveGround(*fluidGroup);
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
	auto fluidDataSetCopy = m_fluid.queryGetOne(point);
	assert(std::ranges::find(fluidDataSetCopy, fluidType, &FluidData::type) != fluidDataSetCopy.end());
	if(fluidDataSetCopy.size() == 1)
		return;
	for(FluidData& fluidData : fluidDataSetCopy)
		if(fluidData.type != fluidType)
			fluidData.group->m_stable = false;
	m_fluid.insert(point, std::move(fluidDataSetCopy));
}
void Space::fluid_drainInternal(const Point3D& point, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	auto& fluidData = m_fluid.queryGetOneMutable(point);
	auto iter = std::ranges::find(fluidData, fluidType, &FluidData::type);
	assert(iter != fluidData.end());
	auto offset = iter - fluidData.begin();
	if(iter->volume == volume)
		// use Vector::erase rather then util::removeFromVectorByValueUnordered despite being slower in order to preserve sort order.
		fluidData.erase(fluidData.begin() + offset);
	else
		fluidData[offset].volume -= volume;
	if(fluidData.empty())
		m_fluid.remove(point);
	m_totalFluidVolume.updateSubtractOne(point, volume);
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.update({point, point});
	floating_maybeSink(point);
	fluid_maybeEraseFluidOnDeck(point);
}
void Space::fluid_fillInternal(const Point3D& point, const CollisionVolume& volume, FluidGroup& fluidGroup)
{
	auto fluids = m_fluid.queryGetOne(point);
	bool wasEmpty = fluids.empty();
	const auto found = std::ranges::find(fluids, fluidGroup.m_fluidType, &FluidData::type);
	if(found == fluids.end())
		fluids.emplace_back(fluidGroup.m_fluidType, &fluidGroup, volume);
	else
		found->volume += volume;
	m_fluid.insert(point, std::move(fluids));
	m_totalFluidVolume.updateAddOne(point, volume);
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.update({point, point});
	floating_maybeFloatUp(point);
	if(wasEmpty)
		fluid_maybeRecordFluidOnDeck(point);
}
void Space::fluid_unsetGroupInternal(const Point3D& point, const FluidTypeId& fluidType)
{
	m_fluid.updateOne(point, [&](std::vector<FluidData>& fluid){
		const auto found = std::ranges::find(fluid, fluidType, &FluidData::type);
		assert(found != fluid.end());
		found->group = nullptr;
	});
}
void Space::fluid_setGroupInternal(const Point3D& point, const FluidTypeId& fluidType, FluidGroup& group)
{
	m_fluid.updateOne(point, [&](std::vector<FluidData>& fluid){
		const auto found = std::ranges::find(fluid, fluidType, &FluidData::type);
		assert(found != fluid.end());
		found->group = &group;
	});
}
bool Space::fluid_undisolveInternal(const Point3D& point, FluidGroup& fluidGroup)
{
	auto& fluids = m_fluid.queryGetOneMutable(point);
	const auto found = std::ranges::find(fluids, fluidGroup.m_fluidType, &FluidData::type);
	// TODO: fluid_canEnterCurrently queries m_fluid again.
	if(found != fluids.end() || fluid_canEnterCurrently(point, fluidGroup.m_fluidType))
	{
		CollisionVolume capacity = fluid_volumeOfTypeCanEnter(point, fluidGroup.m_fluidType);
		assert(fluidGroup.m_excessVolume > 0);
		CollisionVolume flow = std::min(capacity, CollisionVolume::create(fluidGroup.m_excessVolume));
		fluidGroup.m_excessVolume -= flow.get();
		if(found != fluids.end())
		{
			// Merge disolved group into found group.
			found->volume += flow;
			found->group->m_excessVolume += fluidGroup.m_excessVolume;
			fluidGroup.m_destroy = true;
		}
		else
		{
			// Undisolve group.
			m_fluid.updateOne(point, [&](std::vector<FluidData>& data){ data.emplace_back(fluidGroup.m_fluidType, &fluidGroup, flow); });
			fluidGroup.addPoint(m_area, point, false);
			fluidGroup.m_disolved = false;
		}
		m_totalFluidVolume.updateAddOne(point, flow);
		//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
		m_area.m_hasTerrainFacades.update({point, point});
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
	assert(m_totalFluidVolume.queryGetOne(point) >= volume);
	auto& fluids = m_fluid.queryGetOneMutable(point);
	const auto found = std::ranges::find(fluids, fluidType, &FluidData::type);
	if(found->volume > volume)
		found->volume -= volume;
	else
	{
		FluidGroup& group = *found->group;
		if(group.getPoints().size() == 1)
			m_area.m_hasFluidGroups.removeFluidGroup(group);
		else
			group.removePoint(m_area, point);
		fluids.erase(found);
	}
	m_totalFluidVolume.updateSubtractOne(point, volume);
	m_area.m_hasTerrainFacades.update({point, point});
	floating_maybeSink(point);
	fluid_maybeEraseFluidOnDeck(point);
}
void Space::fluid_removeAllSyncronus(const Point3D& point)
{
	auto copy = m_fluid.queryGetOne(point);
	for(FluidData& data : copy)
		fluid_removeSyncronus(point, data.volume, data.type);
	assert(m_totalFluidVolume.queryGetOne(point) == 0);
}
bool Space::fluid_canEnterCurrently(const Point3D& point, const FluidTypeId& fluidType) const
{
	if(m_totalFluidVolume.queryGetOne(point) < Config::maxPointVolume)
		return true;
	for(const FluidData& fluidData : m_fluid.queryGetOne(point))
		if(FluidType::getDensity(fluidData.type) < FluidType::getDensity(fluidType))
			return true;
	return false;
}
bool Space::fluid_canEnterEver(const Point3D& point) const
{
	return !solid_is(point);
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
	for(const FluidData& fluidData : m_fluid.queryGetOne(point))
		if(FluidType::getDensity(fluidData.type) >= FluidType::getDensity(fluidType))
			output -= fluidData.volume;
	return output;
}
CollisionVolume Space::fluid_volumeOfTypeContains(const Point3D& point, const FluidTypeId& fluidType) const
{
	const auto found = fluid_getData(point, fluidType);
	if(!found)
		return {0};
	return found->volume;
}
FluidTypeId Space::fluid_getTypeWithMostVolume(const Point3D& point) const
{
	assert(m_fluid.queryAny(point));
	CollisionVolume volume = {0};
	FluidTypeId output;
	for(const FluidData& fluidData: m_fluid.queryGetOne(point))
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
	CollisionVolume totalFluidVolume = m_totalFluidVolume.queryGetOne(point);
	// Fluid types are sorted by density.
	auto fluidDataSet = fluid_getAllSortedByDensityAscending(point);
	for(FluidData& fluidData : fluidDataSet)
	{

		assert(fluidData.type == fluidData.group->m_fluidType);
		// Displace lower density fluids.
		CollisionVolume displaced = std::min(fluidData.volume, totalFluidVolume - Config::maxPointVolume);
		assert(displaced.exists());
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
	fluid_setTotalVolume(point, totalFluidVolume);
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
	m_area.m_hasTerrainFacades.update({point, point});
}
void Space::fluid_onPointSetSolid(const Point3D& point)
{
	// Displace fluids.
	auto fluidDataSet = m_fluid.queryGetOne(point);
	if(!fluidDataSet.empty())
	{
		for(FluidData& fluidData : fluidDataSet)
		{
			fluidData.group->removePoint(m_area, point);
			fluidData.group->addFluid(m_area, fluidData.volume);
			// If there is no where to put the fluid.
			if(fluidData.group->m_drainQueue.m_set.empty() && fluidData.group->m_fillQueue.m_set.empty())
			{
				// If fluid piston is enabled then find a place above to add to potential.
				if constexpr (Config::fluidPiston)
				{
					Point3D above = point.above();
					while(above.exists())
					{
						if(fluid_canEnterEver(above) && fluid_canEnterCurrently(above, fluidData.type))
						{
							fluidData.group->m_fillQueue.maybeAddPoint(above);
							break;
						}
						above = above.above();
					}
					// The only way that this 'piston' code should be triggered is if something falls, which means the top layer cannot be full.
					assert(above.exists());
				}
				else
					// Otherwise destroy the group.
					m_area.m_hasFluidGroups.removeFluidGroup(*fluidData.group);
			}
			fluidData.group = nullptr;
		}
		m_fluid.remove(point);
		fluid_setTotalVolume(point, {0});
	}
	// Collect adjacent while also removing now solid point from all fill queues of adjacent groups.
	SmallSet<Point3D> candidates;
	for(const Point3D& adjacent : getDirectlyAdjacent(point))
		if(fluid_canEnterEver(adjacent))
		{
			candidates.insert(adjacent);
			auto adjacentFluidDataSet = m_fluid.queryGetOne(adjacent);
			for(FluidData& fluidData : adjacentFluidDataSet)
				fluidData.group->m_fillQueue.maybeRemovePoint(point);
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
		toDisperseRemainder.insert(data.type, remainder);
	}
	for(const Point3D& point : candidates)
		for(const auto& [fluidType, volume] : toDispersePerBlock)
			fluid_add({point, point}, volume, fluidType);
	auto& random = m_area.m_simulation.m_random;
	for(const auto& [fluidType, volume] : toDisperseRemainder)
	{
		Point3D point = random.getInVector(candidates.getVector());
		fluid_add({point, point}, volume, fluidType);
	}
}
void Space::fluid_onPointSetNotSolid(const Point3D& point)
{
	for(const Point3D& adjacent : getDirectlyAdjacent(point))
		if(fluid_canEnterEver(adjacent))
		{
			auto fluidDataSetCopy = m_fluid.queryGetOne(adjacent);
			for(FluidData& fluidData : fluidDataSetCopy)
			{
				fluidData.group->m_fillQueue.maybeAddPoint(point);
				fluidData.group->m_stable = false;
			}
		}
}
bool Space::fluid_contains(const Point3D& point, const FluidTypeId& fluidType) const
{
	const auto& fluid = m_fluid.queryGetOne(point);
	return std::ranges::find(fluid, fluidType, &FluidData::type) != fluid.end();
}
CollisionVolume Space::fluid_getTotalVolume(const Point3D& point) const
{
	auto output = m_totalFluidVolume.queryGetOneOr(point, {0});
	CollisionVolume total = {0};
	for(FluidData data : m_fluid.queryGetOne(point))
		total += data.volume;
	assert(total == output);
	return output;
}
const std::vector<FluidData>& Space::fluid_getAll(const Point3D& point) const
{
	return m_fluid.queryGetOne(point);
}
const std::vector<FluidData> Space::fluid_getAllSortedByDensityAscending(const Point3D& point)
{
	auto fluidData = m_fluid.queryGetOne(point);
	std::ranges::sort(fluidData, std::less<Density>(), [](const auto& data) { return FluidType::getDensity(data.type); });
	return fluidData;
}
bool Space::fluid_any(const Point3D& point) const
{
	return m_fluid.queryAny(point);
}
void Space::fluid_maybeRecordFluidOnDeck(const Point3D& point)
{
	assert(m_totalFluidVolume.queryGetOne(point) != 0);
	const DeckId& deckId = m_area.m_decks.getForPoint(point);
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
	if(m_totalFluidVolume.queryGetOne(point) == 0)
	{
		const DeckId& deckId = m_area.m_decks.getForPoint(point);
		if(deckId.exists())
		{
			const ActorOrItemIndex& isOnDeckOf = m_area.m_decks.getForId(deckId);
			assert(isOnDeckOf.isItem());
			const ItemIndex& item = isOnDeckOf.getItem();
			m_area.getItems().onDeck_erasePointContainingFluid(item, point);
		}
	}
}