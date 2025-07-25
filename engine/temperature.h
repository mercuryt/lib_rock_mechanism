#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "numericTypes/index.h"
#include "reference.h"
#include "simulation/simulation.h"
#include "numericTypes/types.h"
#include "dataStructures/smallSet.h"

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
	Point3D m_point;
	TemperatureDelta m_temperature;
	TemperatureDelta getTemperatureDeltaForRange(const Distance& range);
	void apply(Area& area);
public:
	TemperatureSource() { assert(false); std::unreachable(); }
	TemperatureSource(Area& a, const TemperatureDelta& t, const Point3D& b) : m_point(b), m_temperature(t) { apply(a); }
	void setTemperature(Area& a, const TemperatureDelta& t);
	void unapply(Area& a);
	friend class AreaHasTemperature;
};
//TODO:  move to area directory.
class AreaHasTemperature final
{
	SmallMap<Point3D, TemperatureSource> m_sources;
	// TODO: make these medium maps.
	// TODO: change SmallSet<Point3D> to CuboidSet.
	// To possibly thaw.
	std::map<Temperature, SmallSet<Point3D>> m_aboveGroundPointsByMeltingPoint;
	// To possibly freeze.
	std::map<Temperature, SmallSet<FluidGroup*>> m_aboveGroundFluidGroupsByMeltingPoint;
	// Collect deltas to apply sum.
	SmallMap<Point3D, TemperatureDelta> m_pointDeltaDeltas;
	Area& m_area;
	Temperature m_ambiantSurfaceTemperature;

public:
	AreaHasTemperature(Area& a) : m_area(a) { }
	void setAmbientSurfaceTemperature(const Temperature& temperature);
	void updateAmbientSurfaceTemperature();
	void addTemperatureSource(const Point3D& point, const TemperatureDelta& temperature);
	void removeTemperatureSource(TemperatureSource& temperatureSource);
	void addDelta(const Point3D& point, const TemperatureDelta& delta);
	void applyDeltas();
	void doStep() { applyDeltas(); }
	void addMeltableSolidPointAboveGround(const Point3D& point);
	void removeMeltableSolidPointAboveGround(const Point3D& point);
	void addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	void maybeRemoveFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup);
	[[nodiscard]] TemperatureSource& getTemperatureSourceAt(const Point3D& point);
	[[nodiscard]] const Temperature& getAmbientSurfaceTemperature() const { return m_ambiantSurfaceTemperature; }
	[[nodiscard]] Temperature getDailyAverageAmbientSurfaceTemperature() const;
	[[nodiscard]] auto& getAboveGroundFluidGroupsByMeltingPoint() { return m_aboveGroundFluidGroupsByMeltingPoint; }
	[[nodiscard]] auto& getAboveGroundPointsByMeltingPoint() { return m_aboveGroundPointsByMeltingPoint; }
	// For testing.
	[[nodiscard]] auto& getPointDeltaDeltas() { return m_pointDeltaDeltas; }
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
