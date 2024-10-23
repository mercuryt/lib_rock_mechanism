#pragma once
#include "../objective.h"
#include "../config.h"
#include "eventSchedule.hpp"
struct DeserializationMemo;
class ExterminateObjectiveScheduledEvent;
class ExterminateObjective final : public Objective
{
	BlockIndex m_destination;
	HasScheduledEvent<ExterminateObjectiveScheduledEvent> m_event;
public:
	ExterminateObjective(Area& area, const BlockIndex& destination);
	ExterminateObjective(const Json& data, Area& area, const ActorIndex& actor);
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
	void execute(Simulation&, Area* area) { m_objective.execute(*area, m_actor.getIndex()); }
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
