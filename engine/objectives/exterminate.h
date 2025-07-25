#pragma once
#include "../objective.h"
#include "../config.h"
#include "../area/area.h"
#include "eventSchedule.hpp"
struct DeserializationMemo;
class ExterminateObjectiveScheduledEvent;
class ExterminateObjective final : public Objective
{
	Point3D m_destination;
	HasScheduledEvent<ExterminateObjectiveScheduledEvent> m_event;
public:
	ExterminateObjective(Area& area, const Point3D& destination);
	ExterminateObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex&) { }
	void delay(Area&, const ActorIndex&) { }
	void reset(Area&, const ActorIndex&) { }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "exterminate"; }
	friend class ExterminateObjectiveScheduledEvent;
};
class ExterminateObjectiveScheduledEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	ExterminateObjective& m_objective;
	public:
	ExterminateObjectiveScheduledEvent(Area& area, ExterminateObjective& o, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
