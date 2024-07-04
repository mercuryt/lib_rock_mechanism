#include "temperature.h"
#include "area.h"
#include "datetime.h"
#include "nthAdjacentOffsets.h"
#include "config.h"
#include "objective.h"
#include "objectives/goToSafeTemperature.h"
#include "simulation.h"
#include "terrainFacade.h"
#include "types.h"
#include "plants.h"
#include "actors/actors.h"
#include "items/items.h"
#include "blocks/blocks.h"
#include <cmath>
enum class TemperatureZone { Surface, Underground, LavaSea };
Temperature TemperatureSource::getTemperatureDeltaForRange(Temperature range)
{
	if(range == 0)
		return m_temperature;
	Temperature output = m_temperature / std::pow(range, Config::heatDisipatesAtDistanceExponent);
	if(output <= Config::heatRadianceMinimum)
		return 0;
	return output; // subtract radiance mininum from output to smooth transition at edge of affected zone?
}
void TemperatureSource::apply()
{
	m_area.m_hasTemperature.addDelta(m_block, m_temperature);
	int range = 1;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(BlockIndex block : getNthAdjacentBlocks(m_area, m_block, range))
			m_area.m_hasTemperature.addDelta(block, delta);
		++range;
	}
}
void TemperatureSource::unapply()
{
	m_area.m_hasTemperature.addDelta(m_block, m_temperature * -1);
	int range = 1;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(BlockIndex block : getNthAdjacentBlocks(m_area, m_block, range))
			m_area.m_hasTemperature.addDelta(block, delta * -1);
		++range;
	}
}
void TemperatureSource::setTemperature(const int32_t& t)
{
	unapply();
	m_temperature = t;
	apply();
}
void AreaHasTemperature::addTemperatureSource(BlockIndex block, const Temperature& temperature)
{
	[[maybe_unused]] auto pair = m_sources.try_emplace(block, m_area, temperature, block);
	assert(pair.second);
}
void AreaHasTemperature::removeTemperatureSource(TemperatureSource& temperatureSource)
{
	assert(m_sources.contains(temperatureSource.m_block));
	//TODO: Optimize with iterator.
	m_sources.at(temperatureSource.m_block).unapply();
	m_sources.erase(temperatureSource.m_block);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(BlockIndex block)
{
	return m_sources.at(block);
}
void AreaHasTemperature::addDelta(BlockIndex block, int32_t delta)
{
	m_blockDeltaDeltas[block] += delta;
}
// TODO: optimize by splitting block deltas into different structures for different temperature zones, to avoid having to rerun getAmbientTemperature?
void AreaHasTemperature::applyDeltas()
{
	std::unordered_map<BlockIndex, int32_t> oldDeltaDeltas;
	oldDeltaDeltas.swap(m_blockDeltaDeltas);
	for(auto& [block, deltaDelta] : oldDeltaDeltas)
		m_area.getBlocks().temperature_updateDelta(block, deltaDelta);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(const Temperature& temperature)
{
	m_ambiantSurfaceTemperature = temperature;
	m_area.getPlants().onChangeAmbiantSurfaceTemperature();
	m_area.getActors().onChangeAmbiantSurfaceTemperature();
	m_area.getItems().onChangeAmbiantSurfaceTemperature();
	Blocks& blocks = m_area.getBlocks();
	for(auto& [meltingPoint, toMelt] : m_aboveGroundBlocksByMeltingPoint)
		if(meltingPoint <= temperature)
			for(BlockIndex block : toMelt)
				blocks.temperature_melt(block);
		else
			break;
	for(auto& [meltingPoint, fluidGroups] : m_aboveGroundFluidGroupsByMeltingPoint)
		if(meltingPoint > temperature)
			for(FluidGroup* fluidGroup : fluidGroups)
				for(FutureFlowBlock& futureFlowBlock : fluidGroup->m_drainQueue.m_queue)
					blocks.temperature_freeze(futureFlowBlock.block, fluidGroup->m_fluidType);
		else
			break;
}
void AreaHasTemperature::updateAmbientSurfaceTemperature()
{
	// TODO: Latitude and altitude.
	Temperature dailyAverage = getDailyAverageAmbientSurfaceTemperature();
	static Temperature maxDailySwing = 35;
	static uint32_t hottestHourOfDay = 14;
	int32_t hour = DateTime(m_area.m_simulation.m_step).hour;
	uint32_t hoursFromHottestHourOfDay = std::abs((int32_t)hottestHourOfDay - hour);
	uint32_t halfDay = Config::hoursPerDay / 2;
	setAmbientSurfaceTemperature(dailyAverage + ((maxDailySwing * (std::min(0u, halfDay - hoursFromHottestHourOfDay))) / halfDay) - (maxDailySwing / 2));
}
void AreaHasTemperature::addMeltableSolidBlockAboveGround(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(!blocks.isUnderground(block));
	assert(blocks.solid_is(block));
	m_aboveGroundBlocksByMeltingPoint.at(blocks.solid_get(block).meltingPoint).insert(block);
}
// Must be run before block is set no longer solid if above ground.
void AreaHasTemperature::removeMeltableSolidBlockAboveGround(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.solid_is(block));
	m_aboveGroundBlocksByMeltingPoint.at(blocks.solid_get(block).meltingPoint).erase(block);
}
Temperature AreaHasTemperature::getDailyAverageAmbientSurfaceTemperature() const
{
	// TODO: Latitude and altitude.
	static Temperature yearlyHottestDailyAverage = 300;
	static Temperature yearlyColdestDailyAverage = 280;
	static uint32_t dayOfYearOfSolstice = Config::daysPerYear / 2;
	int32_t day = DateTime(m_area.m_simulation.m_step).day;
	uint32_t daysFromSolstice = std::abs(day - (int32_t)dayOfYearOfSolstice);
	return yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
}
UnsafeTemperatureEvent::UnsafeTemperatureEvent(Area& area, ActorIndex a, const Step start) :
	ScheduledEvent(area.m_simulation, area.getActors().getSpecies(a).stepsTillDieInUnsafeTemperature, start), m_actor(a) { }
void UnsafeTemperatureEvent::execute(Simulation&, Area* area) { area->getActors().die(m_actor, CauseOfDeath::temperature); }
void UnsafeTemperatureEvent::clearReferences(Simulation&, Area* area) { area->getActors().m_needsSafeTemperature.at(m_actor)->m_event.clearPointer(); }
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(Area& area, ActorIndex a) : 
	m_event(area.m_eventSchedule), m_actor(a) { }
	/*
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(const Json& data, ActorIndex a, Simulation& s) : 
	m_event(area.m_eventSchedule), m_actor(a), m_objectiveExists(data["objectiveExists"].get<bool>())
{
	if(data.contains("eventStart"))
		m_event.schedule(m_actor, data["eventStart"].get<Step>());
}
Json ActorNeedsSafeTemperature::toJson() const
{
	Json data;
	data["objectiveExists"] = m_objectiveExists;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
*/
void ActorNeedsSafeTemperature::onChange(Area& area)
{
	Actors& actors = area.getActors();
	actors.grow_updateGrowingStatus(m_actor);
	if(!isSafeAtCurrentLocation(area))
	{
		if(!m_objectiveExists)
		{
			m_objectiveExists = true;
			std::unique_ptr<Objective> objective = std::make_unique<GetToSafeTemperatureObjective>(m_actor);
			actors.objective_addNeed(m_actor, std::move(objective));
		}
		if(!m_event.exists())
			m_event.schedule(area, m_actor);
	}
	else if(m_event.exists())
		m_event.unschedule();
}
bool ActorNeedsSafeTemperature::isSafe(Area& area, Temperature temperature) const
{
	const AnimalSpecies& species = area.getActors().getSpecies(m_actor);
	return temperature >= species.minimumSafeTemperature && temperature <= species.maximumSafeTemperature;
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation(Area& area) const
{
	Actors& actors = area.getActors();
	if(actors.getLocation(m_actor) == BLOCK_INDEX_MAX)
		return true;
	return isSafe(area, area.getBlocks().temperature_get(actors.getLocation(m_actor)));
}
