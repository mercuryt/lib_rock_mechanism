#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "objective.h"
#include "findsPath.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <map>

class Area;
class FluidGroup;
class Actor;
struct FluidType;
class GetToSafeTemperatureThreadedTask;
struct DeserializationMemo;

// Raises and lowers nearby temperature.
class TemperatureSource final
{
	//TODO: reuse reference for fire?
	Area& m_area;
	BlockIndex m_block;
	int32_t m_temperature;
	Temperature getTemperatureDeltaForRange(uint32_t range);
	void apply();
public:
	TemperatureSource(Area& a, const int32_t& t, BlockIndex b) : m_area(a), m_block(b), m_temperature(t) { apply(); }
	void setTemperature(const int32_t& t);
	void unapply();
	friend class AreaHasTemperature;
};
class AreaHasTemperature final
{
	std::unordered_map<BlockIndex, TemperatureSource> m_sources;
	// To possibly thaw.
	std::map<Temperature, std::unordered_set<BlockIndex>> m_aboveGroundBlocksByMeltingPoint;
	// To possibly freeze.
	std::map<Temperature, std::unordered_set<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;
	// Collect deltas to apply sum.
	std::unordered_map<BlockIndex, int32_t> m_blockDeltaDeltas;
	Area& m_area;
	Temperature m_ambiantSurfaceTemperature;

public:
	AreaHasTemperature(Area& a) : m_area(a) { }
	void setAmbientSurfaceTemperature(const Temperature& temperature);
	void updateAmbientSurfaceTemperature();
	void addTemperatureSource(BlockIndex block, const Temperature& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	TemperatureSource& getTemperatureSourceAt(BlockIndex block);
	void addDelta(BlockIndex block, int32_t delta);
	void applyDeltas();
	void addMeltableSolidBlockAboveGround(BlockIndex block);
	void removeMeltableSolidBlockAboveGround(BlockIndex block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	const Temperature& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
	Temperature getDailyAverageAmbientSurfaceTemperature() const;
};
class GetToSafeTemperatureObjective final : public Objective
{
	HasThreadedTask<GetToSafeTemperatureThreadedTask> m_getToSafeTemperatureThreadedTask;
	bool m_noWhereWithSafeTemperatureFound;
public:
	GetToSafeTemperatureObjective(Actor& a);
	GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel() { m_getToSafeTemperatureThreadedTask.maybeCancel(); }
	void delay() { cancel(); }
	void reset();
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GetToSafeTemperature; }
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
class UnsafeTemperatureEvent final : public ScheduledEvent
{
	Actor& m_actor;
public:
	UnsafeTemperatureEvent(Actor& a, const Step start = 0);
	void execute();
	void clearReferences();
};
class ActorNeedsSafeTemperature
{
	HasScheduledEvent<UnsafeTemperatureEvent> m_event; // 2
	Actor& m_actor;
	bool m_objectiveExists = false;
public:
	ActorNeedsSafeTemperature(Actor& a, Simulation& s);
	ActorNeedsSafeTemperature(const Json& data, Actor& a, Simulation& s);
	Json toJson() const;
	void onChange();
	bool isSafe(Temperature temperature) const;
	bool isSafeAtCurrentLocation() const;
	friend class GetToSafeTemperatureObjective;
	friend class UnsafeTemperatureEvent;
	// For UI.
	[[nodiscard]] Percent dieFromTemperaturePercent() const { return isSafeAtCurrentLocation() ? 0 : m_event.percentComplete(); }
};
