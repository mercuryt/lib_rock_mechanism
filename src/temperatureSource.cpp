#include "temperatureSource.h"
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
			block->applyTemperatureDelta(delta);
		++range;
	}
}
void TemperatureSource::unapply()
{
	int range = 0;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->applyTemperatureDelta(-1 * delta);
		++range;
	}
}
void TemperatureSource::setTemperature(int32_t t)
{
	unapply();
	m_temperature = t;
	apply();
}
void HasTemperatureSources::add(Block& block, uint32_t temperature)
{
	auto pair = m_sources.emplace(&block, block, temperature);
	assert(pair.second);
	pair.first->apply()
}
void HasTemperatureSources::remove(Block& block)
{
	assert(m_sources.contains(&block));
	//TODO: Optimize.
	m_sources.at(&block).unapply();
	m_sources.remove(&block);
}
TemperatureSource& at(Block& block)
{
	return m_sources.at(&block);
}
