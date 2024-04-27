#pragma once
#include "../objective.h"
#include "../config.h"
#include "eventSchedule.hpp"
class Actor;
class Block;
struct DeserializationMemo;
class ExterminateObjectiveScheduledEvent;
class ExterminateObjective final : public Objective
{
	Block& m_destination;
	HasScheduledEvent<ExterminateObjectiveScheduledEvent> m_event;
public:
	ExterminateObjective(Actor& a, Block& destination);
	ExterminateObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel() { }
	void delay() { }
	void reset() { }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Exterminate; }
	[[nodiscard]] std::string name() const { return "exterminate"; }
	friend class ExterminateObjectiveScheduledEvent;
};
class ExterminateObjectiveScheduledEvent final : public ScheduledEvent
{
	ExterminateObjective& m_objective;
public:
	ExterminateObjectiveScheduledEvent(ExterminateObjective& o, Step start = 0);
	void execute() { m_objective.execute(); }
	void clearReferences() { m_objective.m_event.clearPointer(); }
};
