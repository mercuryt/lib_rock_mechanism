#include "temperature.h"
#include "block.h"
#include "area.h"
#include "path.h"
#include "nthAdjacentOffsets.h"
#include "config.h"
#include <cmath>
enum class TemperatureZone { Surface, Underground, LavaSea };
uint32_t TemperatureSource::getTemperatureDeltaForRange(uint32_t range)
{
	if(range == 0)
		return m_temperature;
	uint32_t output = m_temperature / std::pow(range, Config::heatDisipatesAtDistanceExponent);
	if(output <= Config::heatRadianceMinimum)
		return 0;
	return output; // subtract radiance mininum from output to smooth transition at edge of affected zone?
}
void TemperatureSource::apply()
{
	int range = 0;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->m_area->m_areaHasTemperature.addDelta(*block, delta);
		++range;
	}
}
void TemperatureSource::unapply()
{
	int range = 0;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->m_area->m_areaHasTemperature.addDelta(*block, delta * -1);
		++range;
	}
}
void TemperatureSource::setTemperature(const int32_t& t)
{
	unapply();
	m_temperature = t;
	apply();
}
void AreaHasTemperature::addTemperatureSource(Block& block, const uint32_t& temperature)
{
	auto pair = m_sources.try_emplace(&block, temperature, block);
	assert(pair.second);
}
void AreaHasTemperature::removeTemperatureSource(TemperatureSource& temperatureSource)
{
	assert(m_sources.contains(&temperatureSource.m_block));
	//TODO: Optimize with iterator.
	m_sources.at(&temperatureSource.m_block).unapply();
	m_sources.erase(&temperatureSource.m_block);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(Block& block)
{
	return m_sources.at(&block);
}
void AreaHasTemperature::addDelta(Block& block, int32_t delta)
{
	m_blockDeltaDeltas[&block] += delta;
}
// TODO: optimize by splitting block deltas into different structures for different temperature zones, to avoid having to rerun getAmbientTemperature?
void AreaHasTemperature::applyDeltas()
{
	std::unordered_map<Block*, int32_t> oldDeltas;
	oldDeltas.swap(m_blockDeltaDeltas);
	for(auto& [block, delta] : oldDeltas)
		block->m_blockHasTemperature.updateDelta(delta);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(const uint32_t& temperature)
{
	m_ambiantSurfaceTemperature = temperature;
	m_area.m_hasPlants.onChangeAmbiantSurfaceTemperature();
	m_area.m_hasActors.onChangeAmbiantSurfaceTemperature();
	m_area.m_hasItems.onChangeAmbiantSurfaceTemperature();
	for(auto& [meltingPoint, blocks] : m_aboveGroundBlocksByMeltingPoint)
		if(meltingPoint <= temperature)
			for(Block* block : blocks)
				block->m_blockHasTemperature.melt();
		else
			break;
	for(auto& [meltingPoint, fluidGroups] : m_aboveGroundFluidGroupsByMeltingPoint)
		if(meltingPoint > temperature)
			for(FluidGroup* fluidGroup : fluidGroups)
				for(FutureFlowBlock& futureFlowBlock : fluidGroup->m_drainQueue.m_queue)
					futureFlowBlock.block->m_blockHasTemperature.freeze(fluidGroup->m_fluidType);
		else
			break;
}
void AreaHasTemperature::setAmbientTemperatureFor(uint32_t hour, uint32_t dayOfYear)
{
	// TODO: generate constants from latitude and altitude.
	static uint32_t yearlyHottestDailyAverage = 300;
	static uint32_t yearlyColdestDailyAverage = 280;
	static uint32_t dayOfYearOfSolstice = Config::daysPerYear / 2;
	uint32_t daysFromSolstice = std::abs((int32_t)dayOfYear - (int32_t)dayOfYearOfSolstice);
	uint32_t dailyAverage = yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
	static uint32_t maxDailySwing = 35;
	static uint32_t hottestHourOfDay = 14;
	uint32_t hoursFromHottestHourOfDay = std::abs((int32_t)hottestHourOfDay - (int32_t)hour);
	uint32_t halfDay =  Config::hoursPerDay / 2;
	setAmbientSurfaceTemperature(dailyAverage + ((maxDailySwing * (halfDay - hoursFromHottestHourOfDay)) / halfDay) - (maxDailySwing / 2));
}
void AreaHasTemperature::addMeltableSolidBlockAboveGround(Block& block)
{
	assert(!block.m_underground);
	assert(block.isSolid());
	m_aboveGroundBlocksByMeltingPoint.at(block.getSolidMaterial().meltingPoint).insert(&block);
}
// Must be run before block is set no longer solid if above ground.
void AreaHasTemperature::removeMeltableSolidBlockAboveGround(Block& block)
{
	assert(block.isSolid());
	m_aboveGroundBlocksByMeltingPoint.at(block.getSolidMaterial().meltingPoint).erase(&block);
}
void BlockHasTemperature::updateDelta(int32_t delta)
{
	m_delta += delta;
	uint32_t temperature = m_delta + getAmbientTemperature();
	if(m_block.isSolid())
	{
		auto& material = m_block.getSolidMaterial();
		if(material.burnData != nullptr && material.burnData->ignitionTemperature <= temperature && m_block.m_fire == nullptr)
			m_block.m_fire = std::make_unique<Fire>(m_block, m_block.getSolidMaterial());
		else if(material.meltingPoint != 0 && material.meltingPoint <= temperature)
			m_block.m_blockHasTemperature.melt();
	}
	else
	{
		m_block.m_hasPlant.setTemperature(temperature);
		m_block.m_hasBlockFeatures.setTemperature(temperature);
		m_block.m_hasActors.setTemperature(temperature);
		m_block.m_hasItems.setTemperature(temperature);
		//TODO: FluidGroups.
	}
}
void BlockHasTemperature::freeze(const FluidType& fluidType)
{
	assert(fluidType.freezesInto != nullptr);
	static const ItemType& chunk = ItemType::byName("chunk");
	uint32_t chunkVolume = m_block.m_hasItems.getCount(chunk, *fluidType.freezesInto);
	uint32_t fluidVolume = m_block.m_fluids.at(&fluidType).first;
	if(chunkVolume + fluidVolume >= Config::maxBlockVolume)
	{
		m_block.setSolid(*fluidType.freezesInto);
		uint32_t remainder = chunkVolume + fluidVolume - Config::maxBlockVolume;
		(void)remainder;
		//TODO: add remainder to fluid group or above block.
	}
	else
		m_block.m_hasItems.add(chunk, *fluidType.freezesInto,  fluidVolume);
}
void BlockHasTemperature::melt()
{
	assert(m_block.isSolid());
	assert(m_block.getSolidMaterial().meltsInto != nullptr);
	m_block.addFluid(Config::maxBlockVolume, *m_block.getSolidMaterial().meltsInto);
}
const uint32_t& BlockHasTemperature::getAmbientTemperature() const 
{
	if(m_block.m_underground)
	{
		if(m_block.m_z <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_block.m_area->m_areaHasTemperature.getAmbientSurfaceTemperature();
}
void GetToSafeTemperatureThreadedTask::readStep()
{
	auto condition = [&](Block& block)
	{
		return m_objective.m_actor.m_needsSafeTemperature.isSafe(block.m_blockHasTemperature.get());
	};
	m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
}
void GetToSafeTemperatureThreadedTask::writeStep()
{
	if(m_result.empty())
		m_objective.m_actor.m_hasObjectives.cannotFulfillNeed(m_objective);
}
void GetToSafeTemperatureObjective::execute()
{
	if(m_actor.m_needsSafeTemperature.isSafeAtCurrentLocation())
		m_actor.m_hasObjectives.objectiveComplete(*this);
	else
		m_getToSafeTemperatureThreadedTask.create(*this);
}
GetToSafeTemperatureObjective::~GetToSafeTemperatureObjective() { m_actor.m_needsSafeTemperature.m_objectiveExists = false; }
void ActorNeedsSafeTemperature::onChange()
{
	m_actor.m_canGrow.updateGrowingStatus();
	if(!m_objectiveExists && !isSafeAtCurrentLocation())
	{
		m_objectiveExists = true;
		std::unique_ptr<Objective> objective = std::make_unique<GetToSafeTemperatureObjective>(m_actor);
		m_actor.m_hasObjectives.addNeed(std::move(objective));
	}
}
bool ActorNeedsSafeTemperature::isSafe(uint32_t temperature) const
{
	return temperature >= m_actor.m_species.minimumSafeTemperature && temperature <= m_actor.m_species.maximumSafeTemperature;
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation() const
{
	return isSafe(m_actor.m_location->m_blockHasTemperature.get());
}
