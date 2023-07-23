#include "temperature.h"
#include "block.h"
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
void TemperatureSource::setTemperature(int32_t t)
{
	unapply();
	m_temperature = t;
	apply();
}
void AreaHasTemperature::addTemperatureSource(Block& block, uint32_t temperature)
{
	auto pair = m_sources.emplace(&block, block, temperature);
	assert(pair.second);
	pair.first->apply()
}
void AreaHasTemperature::removeTemperatureSource(Block& block)
{
	assert(m_sources.contains(&block));
	//TODO: Optimize.
	m_sources.at(&block).unapply();
	m_sources.remove(&block);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(Block& block)
{
	return m_sources.at(&block);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(uint32_t temperature)
{
	m_area.m_hasPlants.setAmbientSurfaceTemperature(temperature);
	m_area.m_hasActors.setAmbientSurfaceTemperature(temperature);
	for(auto& [meltingPoint, blocks] : m_aboveGroundBlocksByMeltingPoint)
		if(meltingPoint <= temperature)
			for(Block* block : blocks)
				block->melt();
		else
			break;
	for(auto& [meltingPoint, blocks] : m_aboveGroundFluidGroupsByMeltingPoint)
		if(meltingPoint > temperature)
			for(Block* block : blocks)
				block->freeze();
		else
			break;
	m_ambiantSurfaceTemperature = temperature;
}
void AreaHasTemperature::addMeltableSolidBlockAboveGround(Block& block)
{
	assert(!block.underground);
	assert(block.isSolid());
	m_aboveGroundBlocksByMeltingPoint.at(block.getSolidMaterial().meltingPoint).insert(&block);
}
// Must be run before block is set no longer solid if above ground.
void AreaHasTemperature::removeMeltableSolidBlockAboveGround(Block& block)
{
	assert(block.isSolid());
	m_aboveGroundBlocksByMeltingPoint.at(block.getSolidMaterial().meltingPoint).remove(&block);
}
void BlockHasTemperature::setDelta(const uint32_t& delta)
{
	uint32_t newTemperature = getAmbiantTemperature() + delta;
	if(m_block.isSolid())
	{
		if(m_block.getSolidMaterial().ignitionTemperature <= newTemperature && m_block.m_fire == nullptr)
			m_block.m_fire = std::make_unique<Fire>(*this, materialType);
		else if(m_block.getSolidMaterial().meltTemperature <= newTemperature)
			m_block.melt();
	}
	else
	{
		m_block.m_hasPlant.setTemperature(newTemperature);
		m_block.m_hasBlockFeatures.setTemperature(newTemperature);
		m_block.m_hasActors.setTemperature(newTemperature);
		m_block.m_hasItems.setTemperature(newTemperature);
		//TODO: FluidGroups.
	}
	m_delta = delta;
}
const uint32_t& BlockHasTemperature::getAmbiantTemperature() const 
{
	if(m_block.m_underground)
	{
		if(m_block.m_z <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_block.m_area->m_areaHasTemperature.getAmbiantSurfaceTemperature();
}
