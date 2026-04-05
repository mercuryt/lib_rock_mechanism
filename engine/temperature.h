#pragma once
#include "eventSchedule.hpp"
#include "numericTypes/index.h"
#include "reference.h"
#include "numericTypes/types.h"
#include "gdb.h"
class UnsafeTemperatureEvent;
class ActorNeedsSafeTemperature
{
	HasScheduledEvent<UnsafeTemperatureEvent> m_event; // 2
	ActorReference m_actor;
public:
	ActorNeedsSafeTemperature(Area& area, const ActorIndex a);
	ActorNeedsSafeTemperature(const Json& data, const ActorIndex a, Area& area);
	void dieFromTemperature(Area& area);
	void unschedule();
	void onChange(Area& area);
	void setTemperature(Area& area, Temperature temperature);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool isSafe(Area& area, const Temperature temperature) const;
	[[nodiscard]] bool isSafeAtCurrentLocation(Area& area) const;
	[[nodiscard]] GDB_CALLABLE bool eventExists();
	friend class GetToSafeTemperatureObjective;
	friend class UnsafeTemperatureEvent;
	// For UI.
	[[nodiscard]] GDB_CALLABLE Percent dieFromTemperaturePercent(Area& area) const;
};
class UnsafeTemperatureEvent final : public ScheduledEvent
{
	ActorNeedsSafeTemperature& m_needsSafeTemperature;
public:
	UnsafeTemperatureEvent(Area& area, const ActorIndex actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};