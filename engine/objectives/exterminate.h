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
	ExterminateObjective(ActorIndex a, BlockIndex destination);
	ExterminateObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel() { }
	void delay() { }
	void reset(Area&) { }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Exterminate; }
	[[nodiscard]] std::string name() const { return "exterminate"; }
	friend class ExterminateObjectiveScheduledEvent;
};
class ExterminateObjectiveScheduledEvent final : public ScheduledEvent
{
	ExterminateObjective& m_objective;
public:
	ExterminateObjectiveScheduledEvent(ExterminateObjective& o, Step start = 0);
	void execute(Simulation&, Area*) { m_objective.execute(); }
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
