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
	ExterminateObjective(Area& area, BlockIndex destination);
	ExterminateObjective(const Json& data, Area& area, ActorIndex actor);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex) { }
	void delay(Area&, ActorIndex) { }
	void reset(Area&, ActorIndex) { }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Exterminate; }
	[[nodiscard]] std::string name() const { return "exterminate"; }
	friend class ExterminateObjectiveScheduledEvent;
};
class ExterminateObjectiveScheduledEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	ExterminateObjective& m_objective;
	public:
	ExterminateObjectiveScheduledEvent(Area& area, ExterminateObjective& o, ActorIndex actor, Step start = Step::create(0));
	void execute(Simulation&, Area* area) { m_objective.execute(*area, m_actor.getIndex()); }
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
