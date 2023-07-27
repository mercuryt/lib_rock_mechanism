#pragma once

#include "nthAdjacentOffsets.h"
#include "eventSchedule.h"
#include "config.h"

#include <unordered_set>
#include <vector>
#include <map>

class Block;
class Area;
class FluidGroup;
class Actor;

// Increases and lowers nearby temperature.
class TemperatureSource final
{
	Block& m_block;
	int32_t m_temperature;
	uint32_t getTemperatureDeltaForRange(uint32_t range);
public:
	TemperatureSource(const int32_t& t, Block& b) : m_block(b), m_temperature(t) { }
	void setTemperature(const int32_t& t);
	void apply();
	void unapply();
	friend class AreaHasTemperature;
};
class AreaHasTemperature final
{
	Area& m_area;
	uint32_t m_ambiantSurfaceTemperature;
	std::unordered_map<Block*, TemperatureSource> m_sources;
	std::map<uint32_t, std::unordered_set<Block*>> m_aboveGroundBlocksByMeltingPoint;
	std::map<uint32_t, std::unordered_set<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;

public:
	AreaHasTemperature(Area& a) : m_area(a) { }
	void setAmbientSurfaceTemperature(const uint32_t& temperature);
	void addTemperatureSource(Block& block, const uint32_t& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	TemperatureSource& getTemperatureSourceAt(Block& block);
	void addMeltableSolidBlockAboveGround(Block& block);
	void removeMeltableSolidBlockAboveGround(Block& block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	const uint32_t& getAmbientSurfaceTemperature() const;
};
class BlockHasTemperature final
{
	Block& m_block;
	uint32_t m_delta;
	void setDelta(const uint32_t& delta);
	const uint32_t& getAmbientTemperature() const;
public:
	BlockHasTemperature(Block& b) : m_block(b) { }
	void addDelta(const uint32_t& delta) { setDelta(m_delta + delta); }
	void subtractDelta(const uint32_t& delta) { setDelta(m_delta - delta); }
	uint32_t getTemperature() const { return m_delta + getAmbientTemperature(); }
};
class ActorNeedsSafeTemperature
{
	Actor& m_actor;
public:
	ActorNeedsSafeTemperature(Actor& a) : m_actor(a) { }
	void setTemperature(uint32_t temperature);
	void callback();
	bool isSafe() const;
};
