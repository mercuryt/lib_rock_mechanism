#pragma once

#include "nthAdjacentOffsets.h"
#include "eventSchedule.h"
#include "config.h"
#include "threadedTask.h"
#include "objective.h"

#include <unordered_set>
#include <vector>
#include <map>

class Block;
class Area;
class FluidGroup;
class Actor;
struct FluidType;
class GetToSafeTemperatureObjective;

// Raises and lowers nearby temperature.
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
	// To possibly thaw.
	std::map<uint32_t, std::unordered_set<Block*>> m_aboveGroundBlocksByMeltingPoint;
	// To possibly freeze.
	std::map<uint32_t, std::unordered_set<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;

public:
	AreaHasTemperature(Area& a, uint32_t ast) : m_area(a), m_ambiantSurfaceTemperature(ast) { }
	void setAmbientSurfaceTemperature(const uint32_t& temperature);
	void addTemperatureSource(Block& block, const uint32_t& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	TemperatureSource& getTemperatureSourceAt(Block& block);
	void addMeltableSolidBlockAboveGround(Block& block);
	void removeMeltableSolidBlockAboveGround(Block& block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	const uint32_t& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
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
	void freeze(const FluidType& fluidType);
	void melt();
	uint32_t get() const { return m_delta + getAmbientTemperature(); }
};
class GetToSafeTemperatureThreadedTask final : public ThreadedTask
{
	GetToSafeTemperatureObjective& m_objective;
	std::vector<Block*> m_result;
public:
	GetToSafeTemperatureThreadedTask(GetToSafeTemperatureObjective& o) : m_objective(o) { }
	void readStep();
	void writeStep();
};
class GetToSafeTemperatureObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<GetToSafeTemperatureThreadedTask> m_getToSafeTemperatureThreadedTask;
public:
	GetToSafeTemperatureObjective(Actor& a) : Objective(Config::getToSafeTemperaturePriority), m_actor(a) { }
	void execute();
	void cancel() { }
	~GetToSafeTemperatureObjective();
	friend class GetToSafeTemperatureThreadedTask;
};
class ActorNeedsSafeTemperature
{
	Actor& m_actor;
	bool m_objectiveExists;
public:
	ActorNeedsSafeTemperature(Actor& a) : m_actor(a), m_objectiveExists(false) { }
	void onChange();
	bool isSafe(uint32_t temperature) const;
	bool isSafeAtCurrentLocation() const;
	friend class GetToSafeTemperatureObjective;
};
