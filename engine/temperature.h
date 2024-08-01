#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "index.h"
#include "reference.h"
#include "simulation.h"
#include "types.h"

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
	TemperatureDelta m_temperature;
	TemperatureDelta getTemperatureDeltaForRange(DistanceInBlocks range);
	void apply();
public:
	TemperatureSource(Area& a, TemperatureDelta t, BlockIndex b) : m_area(a), m_block(b), m_temperature(t) { apply(); }
	void setTemperature(TemperatureDelta t);
	void unapply();
	friend class AreaHasTemperature;
};
class AreaHasTemperature final
{
	std::unordered_map<BlockIndex, TemperatureSource, BlockIndex::Hash> m_sources;
	// To possibly thaw.
	std::map<Temperature, BlockIndices> m_aboveGroundBlocksByMeltingPoint;
	// To possibly freeze.
	std::map<Temperature, std::unordered_set<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;
	// Collect deltas to apply sum.
	std::unordered_map<BlockIndex, TemperatureDelta, BlockIndex::Hash> m_blockDeltaDeltas;
	Area& m_area;
	Temperature m_ambiantSurfaceTemperature;

public:
	AreaHasTemperature(Area& a) : m_area(a) { }
	void setAmbientSurfaceTemperature(Temperature temperature);
	void updateAmbientSurfaceTemperature();
	void addTemperatureSource(BlockIndex block, TemperatureDelta temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	TemperatureSource& getTemperatureSourceAt(BlockIndex block);
	void addDelta(BlockIndex block, TemperatureDelta delta);
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
	[[nodiscard]] Percent dieFromTemperaturePercent(Area& area) const { return isSafeAtCurrentLocation(area) ? Percent::create(0) : m_event.percentComplete(); }
};
class UnsafeTemperatureEvent final : public ScheduledEvent
{
	ActorNeedsSafeTemperature& m_needsSafeTemperature;
public:
	UnsafeTemperatureEvent(Area& area, ActorIndex actor, const Step start = Step::create(0));
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
