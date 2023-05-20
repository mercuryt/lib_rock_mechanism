#pragma once
/*
 * Increases and lowers nearby temperature.
 */

#include "nthAdjacentOffsets.h"

#include <unordered_set>
#include <vector>

template<class Block>
class TemperatureSource
{
	Block& m_block;
	int32_t m_temperature;
	uint32_t getTemperatureDeltaForRange(uint32_t range)
	{
		if(range == 0)
			return m_temperature;
		// Heat disapates at inverse square distance.
		return m_temperature / (range * range);
	}
	void apply()
	{
		int range = 0;
		while(int delta = getTemperatureDeltaForRange(range))
		{
			for(Block* block : getNthAdjacentBlocks(m_block, range))
				block->applyTemperatureDelta(delta);
			++range;
		}
	}
	void unapply()
	{
		int range = 0;
		while(int delta = getTemperatureDeltaForRange(range))
		{
			for(Block* block : getNthAdjacentBlocks(m_block, range))
				block->applyTemperatureDelta(-1 * delta);
			++range;
		}
	}
public:
	TemperatureSource(int32_t t, Block& b) : m_block(b), m_temperature(t)
	{
		apply();
	}
	~TemperatureSource()
	{
		if(!m_block.m_area->m_destroy)
			unapply();
	}
	void setTemperature(int32_t t)
	{
		unapply();
		m_temperature = t;
		apply();
	}
};
