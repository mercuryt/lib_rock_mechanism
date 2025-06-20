#include "temperature.h"
#include "area/area.h"
#include "datetime.h"
#include "blocks/nthAdjacentOffsets.h"
#include "config.h"
#include "objective.h"
#include "objectives/getToSafeTemperature.h"
#include "simulation/simulation.h"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"
#include "plants.h"
#include "actors/actors.h"
#include "items/items.h"
#include "blocks/blocks.h"
#include "definitions/animalSpecies.h"
#include <cmath>
enum class TemperatureZone { Surface, Underground, LavaSea };
TemperatureDelta TemperatureSource::getTemperatureDeltaForRange(const DistanceInBlocks& range)
{
	if(range == 0)
		return m_temperature;
	TemperatureDelta output = m_temperature / std::pow(range.get(), Config::heatDisipatesAtDistanceExponent);
	if(output.absoluteValue() <= Config::heatRadianceMinimum)
		return TemperatureDelta::create(0);
	return output; // subtract radiance mininum from output to smooth transition at edge of affected zone?
}
void TemperatureSource::apply(Area& area)
{
	Blocks& blocks = area.getBlocks();
	area.m_hasTemperature.addDelta(m_block, m_temperature);
	DistanceInBlocks range = DistanceInBlocks::create(1);
	while(range <= Config::heatDistanceMaximum)
	{
		TemperatureDelta delta = getTemperatureDeltaForRange(range);
		if(delta == 0)
			break;
		for(const BlockIndex& block : blocks.getNthAdjacent(m_block, range))
			area.m_hasTemperature.addDelta(block, delta);
		++range;
	}
}
void TemperatureSource::unapply(Area& area)
{
	Blocks& blocks = area.getBlocks();
	area.m_hasTemperature.addDelta(m_block, m_temperature * -1);
	DistanceInBlocks range = DistanceInBlocks::create(1);
	while(true)
	{
		TemperatureDelta delta = getTemperatureDeltaForRange(range);
		if(delta == 0)
			break;
		for(const BlockIndex& block : blocks.getNthAdjacent(m_block, range))
			area.m_hasTemperature.addDelta(block, delta * -1);
		++range;
	}
}
void TemperatureSource::setTemperature(Area& area, const TemperatureDelta& t)
{
	unapply(area);
	m_temperature = t;
	apply(area);
}
void AreaHasTemperature::addTemperatureSource(const BlockIndex& block, const TemperatureDelta& temperature)
{
	m_sources.emplace(block, m_area, temperature, block);
}
void AreaHasTemperature::removeTemperatureSource(TemperatureSource& temperatureSource)
{
	assert(m_sources.contains(temperatureSource.m_block));
	//TODO: Optimize with iterator.
	m_sources[temperatureSource.m_block].unapply(m_area);
	m_sources.erase(temperatureSource.m_block);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(const BlockIndex& block)
{
	return m_sources[block];
}
void AreaHasTemperature::addDelta(const BlockIndex& block, const TemperatureDelta& delta)
{
	auto found = m_blockDeltaDeltas.find(block);
	if(found == m_blockDeltaDeltas.end())
		m_blockDeltaDeltas.insert(block, const_cast<TemperatureDelta&>(delta));
	else
		found->second += delta;
}
// TODO: optimize by splitting block deltas into different structures for different temperature zones, to avoid having to rerun getAmbientTemperature?
void AreaHasTemperature::applyDeltas()
{
	SmallMap<BlockIndex, TemperatureDelta> oldDeltaDeltas;
	oldDeltaDeltas.swap(m_blockDeltaDeltas);
	for(auto& [block, deltaDelta] : oldDeltaDeltas)
		m_area.getBlocks().temperature_updateDelta(block, deltaDelta);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(const Temperature& temperature)
{
	m_ambiantSurfaceTemperature = temperature;
	Blocks& blocks = m_area.getBlocks();
	m_area.getPlants().onChangeAmbiantSurfaceTemperature();
	m_area.getActors().onChangeAmbiantSurfaceTemperature();
	m_area.getItems().onChangeAmbiantSurfaceTemperature();
	m_area.m_exteriorPortals.onChangeAmbiantSurfaceTemperature(blocks, temperature);
	for(auto& [meltingPoint, toMeltOnSurface] : m_aboveGroundBlocksByMeltingPoint)
		if(meltingPoint <= temperature)
		{
			SmallSet<BlockIndex> toMelt;
			for(const BlockIndex& block : toMeltOnSurface)
			{
				// This block has been processed already as part of another block's group.
				if(toMelt.contains(block))
					continue;
				const MaterialTypeId& materialType = blocks.solid_get(block);
				for(const BlockIndex& groupBlock : blocks.collectAdjacentsWithCondition(block, [&](const BlockIndex& b){ return blocks.solid_get(b) == materialType; }))
					toMelt.maybeInsert(groupBlock);
			}
			for(const BlockIndex& block : toMelt)
				blocks.temperature_melt(block);
			m_aboveGroundBlocksByMeltingPoint[meltingPoint].clear();
		}
		else
			break;
	SmallMap<FluidTypeId, SmallSet<BlockIndex>> toFreeze;
	for(auto& [meltingPoint, fluidGroups] : m_aboveGroundFluidGroupsByMeltingPoint)
	{
		const FluidTypeId& fluidType = fluidGroups.front()->m_fluidType;
		// TODO: replace getOrCreate with create.
		auto& blockSet = toFreeze.getOrCreate(fluidType);
		if(meltingPoint > temperature)
		{
			for(FluidGroup* fluidGroup : fluidGroups)
			{
				for(FutureFlowBlock& futureFlowBlock : fluidGroup->m_drainQueue.m_queue)
					if(blocks.isExposedToSky(futureFlowBlock.block))
						blockSet.insert(futureFlowBlock.block);
				fluidGroup->m_aboveGround = false;
			}
			for(const auto& [fluidType, blockSet] : toFreeze)
				for(const BlockIndex& block : blockSet)
					blocks.temperature_freeze(block, fluidType);
			// Any possible freezing at this temperature has happened so clear groupsByMeltingPoint for this temperature.
			// Don't delete the empty set, it will be reused eventually.
			fluidGroups.clear();
		}
		else
			break;
	}
}
void AreaHasTemperature::updateAmbientSurfaceTemperature()
{
	// TODO: Latitude and altitude.
	Temperature dailyAverage = getDailyAverageAmbientSurfaceTemperature();
	static Temperature maxDailySwing = Temperature::create(35);
	static uint32_t hottestHourOfDay = 14;
	int32_t hour = DateTime(m_area.m_simulation.m_step).hour;
	int32_t hoursFromHottestHourOfDay = std::abs((int32_t)hottestHourOfDay - hour);
	int32_t halfDay = Config::hoursPerDay / 2;
	setAmbientSurfaceTemperature(dailyAverage + ((maxDailySwing * (std::max(0, halfDay - hoursFromHottestHourOfDay))) / halfDay) - (maxDailySwing / 2));
}
void AreaHasTemperature::addMeltableSolidBlockAboveGround(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.isExposedToSky(block));
	assert(blocks.solid_is(block));
	m_aboveGroundBlocksByMeltingPoint[MaterialType::getMeltingPoint(blocks.solid_get(block))].insert(block);
}
// Must be run before block is set no longer solid if above ground.
void AreaHasTemperature::removeMeltableSolidBlockAboveGround(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.solid_is(block));
	m_aboveGroundBlocksByMeltingPoint.at(MaterialType::getMeltingPoint(blocks.solid_get(block))).erase(block);
}
void AreaHasTemperature::addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup)
{
	const Temperature& freezing = FluidType::getFreezingPoint(fluidGroup.m_fluidType);
	m_aboveGroundFluidGroupsByMeltingPoint[freezing].insert(&fluidGroup);
}
void AreaHasTemperature::maybeRemoveFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup)
{
	const Temperature& freezing = FluidType::getFreezingPoint(fluidGroup.m_fluidType);
	assert(m_aboveGroundFluidGroupsByMeltingPoint.contains(freezing));
	auto& groups = m_aboveGroundFluidGroupsByMeltingPoint[freezing];
	m_aboveGroundFluidGroupsByMeltingPoint[freezing].maybeErase(&fluidGroup);
	if(groups.empty())
		m_aboveGroundFluidGroupsByMeltingPoint.erase(freezing);
}
Temperature AreaHasTemperature::getDailyAverageAmbientSurfaceTemperature() const
{
	// TODO: Latitude and altitude.
	static Temperature yearlyHottestDailyAverage = Temperature::create(290);
	static Temperature yearlyColdestDailyAverage = Temperature::create(270);
	static uint32_t dayOfYearOfSolstice = Config::daysPerYear / 2;
	int32_t day = DateTime(m_area.m_simulation.m_step).day;
	uint32_t daysFromSolstice = std::abs(day - (int32_t)dayOfYearOfSolstice);
	return yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
}
UnsafeTemperatureEvent::UnsafeTemperatureEvent(Area& area, const ActorIndex& a, const Step start) :
	ScheduledEvent(area.m_simulation, AnimalSpecies::getStepsTillDieInUnsafeTemperature(area.getActors().getSpecies(a)), start),
	m_needsSafeTemperature(*area.getActors().m_needsSafeTemperature[a].get()) { }
void UnsafeTemperatureEvent::execute(Simulation&, Area* area) { m_needsSafeTemperature.dieFromTemperature(*area); }
void UnsafeTemperatureEvent::clearReferences(Simulation&, Area*) { m_needsSafeTemperature.m_event.clearPointer(); }
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(Area& area, const ActorIndex& a) :
	m_event(area.m_eventSchedule)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(const Json& data, const ActorIndex& actor, Area& area) :
	m_event(area.m_eventSchedule)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
	if(data.contains("eventStart"))
		m_event.schedule(area, actor, data["eventStart"].get<Step>());
}
Json ActorNeedsSafeTemperature::toJson() const
{
	Json data;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
void ActorNeedsSafeTemperature::onChange(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	actors.grow_updateGrowingStatus(actor);
	if(!isSafeAtCurrentLocation(area))
	{
		if(!actors.objective_hasNeed(actor, NeedType::temperature))
		{
			std::unique_ptr<Objective> objective = std::make_unique<GetToSafeTemperatureObjective>();
			actors.objective_addNeed(actor, std::move(objective));
		}
		if(!m_event.exists())
			m_event.schedule(area, actor);
	}
	else if(m_event.exists())
		m_event.unschedule();
}
void ActorNeedsSafeTemperature::dieFromTemperature(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	actors.die(actor, CauseOfDeath::temperature);
}
void ActorNeedsSafeTemperature::unschedule()
{
	m_event.unschedule();
}
bool ActorNeedsSafeTemperature::isSafe(Area& area, const Temperature& temperature) const
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	AnimalSpeciesId species = actors.getSpecies(actor);
	return temperature >= AnimalSpecies::getMinimumSafeTemperature(species) && temperature <= AnimalSpecies::getMaximumSafeTemperature(species);
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation(Area& area) const
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	if(actors.getLocation(actor).empty())
		return true;
	return isSafe(area, area.getBlocks().temperature_get(actors.getLocation(actor)));
}