#pragma once
#include "eventSchedule.hpp"
#include "objective.h"
#include "types.h"
#include <string>

class WaitScheduledEvent;
struct DeserializationMemo;

class WaitObjective final : public Objective
{
	HasScheduledEvent<WaitScheduledEvent> m_event;
public:
	// Priority of waiting is 0.
	WaitObjective(Area& area, ActorIndex actor, Step duration);
	WaitObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void delay(Area& area) { reset(area); }
	void cancel(Area& area) { reset(area); }
	void reset(Area& area);
	std::string name() const { return "wait"; }
	[[nodiscard]] bool canResume() const { return false; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Wait; }
	friend class WaitScheduledEvent;
};
class WaitScheduledEvent final : public ScheduledEvent
{
	WaitObjective& m_objective;
public:
	WaitScheduledEvent(Step delay, Area& area, WaitObjective& wo, const Step start = 0);
	void execute(Simulation&, Area* area) { m_objective.execute(*area); }
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
