#pragma once
/*
 * Increases and lowers nearby temperature.
 */

#include "nthAdjacentOffsets.h"

#include <unordered_set>
#include <vector>

class TemperatureSource
{
	Block& m_block;
	int32_t m_temperature;
	uint32_t getTemperatureDeltaForRange(uint32_t range);
	void apply();
public:
	TemperatureSource(int32_t t, Block& b) : m_block(b), m_temperature(t);
	void setTemperature(int32_t t);
	void unapply();
};

class HasTemperatureSources
{
	std::unordered_map<Block*, TemperatureSource> m_sources;
public:
	void add(Block& block, uint32_t temperature);
	void remove(Block& block);
	TemperatureSource& at(Block& block);
};
