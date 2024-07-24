#pragma once
#include "eventSchedule.hpp"
#include "objective.h"
#include "reference.h"
#include "types.h"
#include <string>

class WaitScheduledEvent;
struct DeserializationMemo;

class WaitObjective final : public Objective
{
	HasScheduledEvent<WaitScheduledEvent> m_event;
public:
	// Priority of waiting is 0.
	WaitObjective(Area& area, Step duration, ActorIndex actor);
	WaitObjective(const Json& data, Area& area, ActorIndex actor);
	void execute(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { reset(area, actor); }
	void cancel(Area& area, ActorIndex actor) { reset(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "wait"; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canResume() const { return false; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Wait; }
	friend class WaitScheduledEvent;
};
class WaitScheduledEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	WaitObjective& m_objective;
public:
	WaitScheduledEvent(Step delay, Area& area, WaitObjective& wo, ActorIndex actor, const Step start = 0);
	void execute(Simulation&, Area* area) { m_objective.execute(*area, m_actor.getIndex()); }
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
