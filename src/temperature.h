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
	void apply();
public:
	TemperatureSource(const int32_t& t, Block& b) : m_block(b), m_temperature(t) { apply(); }
	void setTemperature(const int32_t& t);
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
	// Collect deltas to apply sum.
	std::unordered_map<Block*, int32_t> m_blockDeltaDeltas;

public:
	AreaHasTemperature(Area& a, uint32_t ast) : m_area(a), m_ambiantSurfaceTemperature(ast) { }
	void setAmbientSurfaceTemperature(const uint32_t& temperature);
	void setAmbientTemperatureFor(uint32_t hour, uint32_t dayOfYear);
	void addTemperatureSource(Block& block, const uint32_t& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	TemperatureSource& getTemperatureSourceAt(Block& block);
	void addDelta(Block& block, int32_t delta);
	void applyDeltas();
	void addMeltableSolidBlockAboveGround(Block& block);
	void removeMeltableSolidBlockAboveGround(Block& block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	const uint32_t& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
};
class BlockHasTemperature final
{
	Block& m_block;
	// Store to display to user and for pathing to safe temperature.
	uint32_t m_delta;
public:
	BlockHasTemperature(Block& b) : m_block(b), m_delta(0) { }
	void freeze(const FluidType& fluidType);
	void melt();
	void apply(uint32_t temperature, const int32_t& delta);
	void updateDelta(int32_t delta);
	const uint32_t& getAmbientTemperature() const;
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
