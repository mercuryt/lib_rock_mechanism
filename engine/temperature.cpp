#include "temperature.h"
#include "area.h"
#include "datetime.h"
#include "nthAdjacentOffsets.h"
#include "config.h"
#include "objective.h"
#include "objectives/getToSafeTemperature.h"
#include "simulation.h"
#include "terrainFacade.h"
#include "types.h"
#include "plants.h"
#include "actors/actors.h"
#include "items/items.h"
#include "blocks/blocks.h"
#include "animalSpecies.h"
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
	area.m_hasTemperature.addDelta(m_block, m_temperature);
	DistanceInBlocks range = DistanceInBlocks::create(1);
	while(true)
	{
		TemperatureDelta delta = getTemperatureDeltaForRange(range);
		if(delta == 0)
			break;
		for(BlockIndex block : getNthAdjacentBlocks(area, m_block, range.get()))
			area.m_hasTemperature.addDelta(block, delta);
		++range;
	}
}
void TemperatureSource::unapply(Area& area)
{
	area.m_hasTemperature.addDelta(m_block, m_temperature * -1);
	DistanceInBlocks range = DistanceInBlocks::create(1);
	while(true)
	{
		TemperatureDelta delta = getTemperatureDeltaForRange(range);
		if(delta == 0)
			break;
		for(BlockIndex block : getNthAdjacentBlocks(area, m_block, range.get()))
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
	BlockIndexMap<TemperatureDelta> oldDeltaDeltas;
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
	assert(!blocks.isUnderground(block));
	assert(blocks.solid_is(block));
	m_aboveGroundBlocksByMeltingPoint.at(MaterialType::getMeltingPoint(blocks.solid_get(block))).add(block);
}
// Must be run before block is set no longer solid if above ground.
void AreaHasTemperature::removeMeltableSolidBlockAboveGround(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.solid_is(block));
	m_aboveGroundBlocksByMeltingPoint.at(MaterialType::getMeltingPoint(blocks.solid_get(block))).remove(block);
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
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
}
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(const Json& data, const ActorIndex& actor, Area& area) : 
	m_event(area.m_eventSchedule)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
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
	ActorIndex actor = m_actor.getIndex();
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
	ActorIndex actor = m_actor.getIndex();
	actors.die(actor, CauseOfDeath::temperature);
}
void ActorNeedsSafeTemperature::unschedule()
{
	m_event.unschedule();
}
bool ActorNeedsSafeTemperature::isSafe(Area& area, const Temperature& temperature) const
{
	ActorIndex actor = m_actor.getIndex();
	AnimalSpeciesId species = area.getActors().getSpecies(actor);
	return temperature >= AnimalSpecies::getMinimumSafeTemperature(species) && temperature <= AnimalSpecies::getMaximumSafeTemperature(species);
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation(Area& area) const
{
	ActorIndex actor = m_actor.getIndex();
	Actors& actors = area.getActors();
	if(actors.getLocation(actor).empty())
		return true;
	return isSafe(area, area.getBlocks().temperature_get(actors.getLocation(actor)));
}
