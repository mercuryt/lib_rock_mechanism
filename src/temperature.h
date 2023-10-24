#pragma once

#include "nthAdjacentOffsets.h"
#include "eventSchedule.hpp"
#include "config.h"
#include "threadedTask.hpp"
#include "objective.h"
#include "datetime.h"
#include "findsPath.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <map>

class Block;
class Area;
class FluidGroup;
class Actor;
struct FluidType;
class GetToSafeTemperatureThreadedTask;

// Raises and lowers nearby temperature.
class TemperatureSource final
{
	Block& m_block;
	int32_t m_temperature;
	Temperature getTemperatureDeltaForRange(uint32_t range);
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
	Temperature m_ambiantSurfaceTemperature;
	std::unordered_map<Block*, TemperatureSource> m_sources;
	// To possibly thaw.
	std::map<Temperature, std::unordered_set<Block*>> m_aboveGroundBlocksByMeltingPoint;
	// To possibly freeze.
	std::map<Temperature, std::unordered_set<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;
	// Collect deltas to apply sum.
	std::unordered_map<Block*, int32_t> m_blockDeltaDeltas;

public:
	AreaHasTemperature(Area& a) : m_area(a) { }
	void setAmbientSurfaceTemperature(const Temperature& temperature);
	void setAmbientSurfaceTemperatureFor(DateTime now);
	void addTemperatureSource(Block& block, const Temperature& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	TemperatureSource& getTemperatureSourceAt(Block& block);
	void addDelta(Block& block, int32_t delta);
	void applyDeltas();
	void addMeltableSolidBlockAboveGround(Block& block);
	void removeMeltableSolidBlockAboveGround(Block& block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	const Temperature& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
	Temperature getDailyAverageAmbientSurfaceTemperature(DateTime dateTime) const;
};
class BlockHasTemperature final
{
	Block& m_block;
	// Store to display to user and for pathing to safe temperature.
	Temperature m_delta;
public:
	BlockHasTemperature(Block& b) : m_block(b), m_delta(0) { }
	void freeze(const FluidType& fluidType);
	void melt();
	void apply(Temperature temperature, const int32_t& delta);
	void updateDelta(int32_t delta);
	const Temperature& getAmbientTemperature() const;
	Temperature getDailyAverageAmbientTemperature() const;
	Temperature get() const { return m_delta + getAmbientTemperature(); }
};
class GetToSafeTemperatureObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<GetToSafeTemperatureThreadedTask> m_getToSafeTemperatureThreadedTask;
	bool m_noWhereWithSafeTemperatureFound;
public:
	GetToSafeTemperatureObjective(Actor& a);
	void execute();
	void cancel() { m_getToSafeTemperatureThreadedTask.maybeCancel(); }
	void delay() { cancel(); }
	void reset();
	ObjectiveId getObjectiveId() const { return ObjectiveId::GetToSafeTemperature; }
	std::string name() const { return "get to safe temperature"; }
	~GetToSafeTemperatureObjective();
	friend class GetToSafeTemperatureThreadedTask;
};
class GetToSafeTemperatureThreadedTask final : public ThreadedTask
{
	GetToSafeTemperatureObjective& m_objective;
	FindsPath m_findsPath;
	bool m_noWhereWithSafeTemperatureFound;
public:
	GetToSafeTemperatureThreadedTask(GetToSafeTemperatureObjective& o);
	void readStep();
	void writeStep();
	void clearReferences();
};
class UnsafeTemperatureEvent final : public ScheduledEventWithPercent
{
	Actor& m_actor;
public:
	UnsafeTemperatureEvent(Actor& a);
	void execute();
	void clearReferences();
};
class ActorNeedsSafeTemperature
{
	Actor& m_actor;
	HasScheduledEvent<UnsafeTemperatureEvent> m_event;
	bool m_objectiveExists;
public:
	ActorNeedsSafeTemperature(Actor& a);
	void onChange();
	bool isSafe(Temperature temperature) const;
	bool isSafeAtCurrentLocation() const;
	friend class GetToSafeTemperatureObjective;
	friend class UnsafeTemperatureEvent;
};
