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
	ExterminateObjective(Area& area, ActorIndex a, BlockIndex destination);
	ExterminateObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area&);
	void cancel(Area&) { }
	void delay(Area&) { }
	void reset(Area&) { }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Exterminate; }
	[[nodiscard]] std::string name() const { return "exterminate"; }
	friend class ExterminateObjectiveScheduledEvent;
};
class ExterminateObjectiveScheduledEvent final : public ScheduledEvent
{
	ExterminateObjective& m_objective;
public:
	ExterminateObjectiveScheduledEvent(Simulation& simulation, ExterminateObjective& o, Step start = 0);
	void execute(Simulation&, Area* area) { m_objective.execute(*area); }
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
