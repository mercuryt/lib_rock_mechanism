#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "index.h"
#include "reference.h"
#include "simulation.h"
#include "types.h"
#include "vectorContainers.h"

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
	BlockIndex m_block;
	TemperatureDelta m_temperature;
	TemperatureDelta getTemperatureDeltaForRange(const DistanceInBlocks& range);
	void apply(Area& area);
public:
	TemperatureSource(Area& a, const TemperatureDelta& t, const BlockIndex& b) : m_block(b), m_temperature(t) { apply(a); }
	void setTemperature(Area& a, const TemperatureDelta& t);
	void unapply(Area& a);
	friend class AreaHasTemperature;
};
class AreaHasTemperature final
{
	BlockIndexMap<TemperatureSource> m_sources;
	// TODO: make these medium maps.
	// To possibly thaw.
	std::map<Temperature, BlockIndices> m_aboveGroundBlocksByMeltingPoint;
	// To possibly freeze.
	std::map<Temperature, SmallSet<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;
	// Collect deltas to apply sum.
	BlockIndexMap<TemperatureDelta> m_blockDeltaDeltas;
	Area& m_area;
	Temperature m_ambiantSurfaceTemperature;

public:
	AreaHasTemperature(Area& a) : m_area(a) { }
	void setAmbientSurfaceTemperature(const Temperature& temperature);
	void updateAmbientSurfaceTemperature();
	void addTemperatureSource(const BlockIndex& block, const TemperatureDelta& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	void addDelta(const BlockIndex& block, const TemperatureDelta& delta);
	void applyDeltas();
	void doStep() { applyDeltas(); }
	void addMeltableSolidBlockAboveGround(const BlockIndex& block);
	void removeMeltableSolidBlockAboveGround(const BlockIndex& block);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void removeFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	[[nodiscard]] TemperatureSource& getTemperatureSourceAt(const BlockIndex& block);
	[[nodiscard]] const Temperature& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
	[[nodiscard]] Temperature getDailyAverageAmbientSurfaceTemperature() const;
	[[nodiscard]] auto& getAboveGroundFluidGroupsByMeltingPoint() { return m_aboveGroundFluidGroupsByMeltingPoint; }
};
class UnsafeTemperatureEvent;
class ActorNeedsSafeTemperature
{
	HasScheduledEvent<UnsafeTemperatureEvent> m_event; // 2
	ActorReference m_actor;
public:
	ActorNeedsSafeTemperature(Area& area, const ActorIndex& a);
	ActorNeedsSafeTemperature(const Json& data, const ActorIndex& a, Area& area);
	void dieFromTemperature(Area& area);
	void unschedule();
	void onChange(Area& area);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool isSafe(Area& area, const Temperature& temperature) const;
	[[nodiscard]] bool isSafeAtCurrentLocation(Area& area) const;
	friend class GetToSafeTemperatureObjective;
	friend class UnsafeTemperatureEvent;
	// For UI.
	[[nodiscard]] Percent dieFromTemperaturePercent(Area& area) const { return isSafeAtCurrentLocation(area) ? Percent::create(0) : m_event.percentComplete(); }
};
class UnsafeTemperatureEvent final : public ScheduledEvent
{
	ActorNeedsSafeTemperature& m_needsSafeTemperature;
public:
	UnsafeTemperatureEvent(Area& area, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
