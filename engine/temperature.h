#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "reference.h"
#include "simulation.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <map>

class Area;
class FluidGroup;
struct FluidType;
class GetToSafeTemperaturePathRequest;
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
	std::map<Temperature, BlockIndices> m_aboveGroundBlocksByMeltingPoint;
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
	void doStep() { applyDeltas(); }
	void addMeltableSolidBlockAboveGround(BlockIndex block);
	void removeMeltableSolidBlockAboveGround(BlockIndex block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	const Temperature& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
	Temperature getDailyAverageAmbientSurfaceTemperature() const;
};
class UnsafeTemperatureEvent;
class ActorNeedsSafeTemperature
{
	HasScheduledEvent<UnsafeTemperatureEvent> m_event; // 2
	ActorReference m_actor;
public:
	ActorNeedsSafeTemperature(Area& area, ActorIndex a);
	ActorNeedsSafeTemperature(const Json& data, ActorIndex a, Area& area);
	void dieFromTemperature();
	[[nodiscard]] Json toJson() const;
	void onChange(Area& area);
	bool isSafe(Area& area, Temperature temperature) const;
	bool isSafeAtCurrentLocation(Area& area) const;
	friend class GetToSafeTemperatureObjective;
	friend class UnsafeTemperatureEvent;
	// For UI.
	[[nodiscard]] Percent dieFromTemperaturePercent(Area& area) const { return isSafeAtCurrentLocation(area) ? 0 : m_event.percentComplete(); }
};
class UnsafeTemperatureEvent final : public ScheduledEvent
{
	ActorNeedsSafeTemperature& m_needsSafeTemperature;
public:
	UnsafeTemperatureEvent(Area& area, ActorIndex actor, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
