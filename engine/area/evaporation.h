#pragma once
#include "../eventSchedule.hpp"
class EvaporationEvent;
class Area;
class Simulation;
class AreaHasEvaporation
{
	HasScheduledEvent<EvaporationEvent> m_event;
public:
	AreaHasEvaporation(Area& area);
	void schedule(Area& area);
	void execute(Area& area);
	[[nodiscard]] float rateModifierForTemperature(Area& area) const;
	friend class EvaporationEvent;
};
class EvaporationEvent final : public ScheduledEvent
{
	Area& m_area;
public:
	EvaporationEvent(Area& a, const Step& delay);
	void execute(Simulation& simulation, Area* area) override;
	void clearReferences(Simulation& simulation, Area* area) override;
};
