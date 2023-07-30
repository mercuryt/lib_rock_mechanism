#include "temperature.h"
#include "block.h"
#include "area.h"
#include "nthAdjacentOffsets.h"
enum class TemperatureZone { Surface, Underground, LavaSea};
uint32_t TemperatureSource::getTemperatureDeltaForRange(uint32_t range)
{
	if(range == 0)
		return m_temperature;
	// Heat disapates at inverse square distance.
	return m_temperature / (range * range);
}
void TemperatureSource::apply()
{
	int range = 0;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->m_blockHasTemperature.addDelta(delta);
		++range;
	}
}
void TemperatureSource::unapply()
{
	int range = 0;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->m_blockHasTemperature.subtractDelta(delta);
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
	pair.first->second.apply();
}
void AreaHasTemperature::removeTemperatureSource(TemperatureSource& temperatureSource)
{
	assert(m_sources.contains(&temperatureSource.m_block));
	//TODO: Optimize.
	m_sources.at(&temperatureSource.m_block).unapply();
	m_sources.erase(&temperatureSource.m_block);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(Block& block)
{
	return m_sources.at(&block);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(const uint32_t& temperature)
{
	m_ambiantSurfaceTemperature = temperature;
	m_area.m_hasPlants.onChangeAmbiantSurfaceTemperature();
	m_area.m_hasActors.onChangeAmbiantSurfaceTemperature();
	//m_area.m_hasItems.onChangeAmbiantSurfaceTemperature();
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
void BlockHasTemperature::setDelta(const uint32_t& delta)
{
	uint32_t newTemperature = getAmbientTemperature() + delta;
	m_delta = delta;
	if(m_block.isSolid())
	{
		if(m_block.getSolidMaterial().ignitionTemperature <= newTemperature && m_block.m_fire == nullptr)
			m_block.m_fire = std::make_unique<Fire>(m_block, m_block.getSolidMaterial());
		else if(m_block.getSolidMaterial().meltingPoint <= newTemperature)
			m_block.m_blockHasTemperature.melt();
	}
	else
	{
		m_block.m_hasPlant.setTemperature(newTemperature);
		m_block.m_hasBlockFeatures.setTemperature(newTemperature);
		m_block.m_hasActors.setTemperature(newTemperature);
		m_block.m_hasItems.setTemperature(newTemperature);
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
void ActorNeedsSafeTemperature::onChange()
{
	m_actor.m_canGrow.updateGrowingStatus();
}
bool ActorNeedsSafeTemperature::isSafe(uint32_t temperature) const
{
	return temperature >= m_actor.m_species.minimumSafeTemperature && temperature <= m_actor.m_species.maximumSafeTemperature;
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation() const
{
	return isSafe(m_actor.m_location->m_blockHasTemperature.get());
}
