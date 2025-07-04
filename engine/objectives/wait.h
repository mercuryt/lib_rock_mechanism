#pragma once
#include "eventSchedule.hpp"
#include "objective.h"
#include "reference.h"
#include "numericTypes/types.h"
#include <string>

class WaitScheduledEvent;
struct DeserializationMemo;

class WaitObjective final : public Objective
{
	HasScheduledEvent<WaitScheduledEvent> m_event;
public:
	// Priority of waiting is 0.
	WaitObjective(Area& area, const Step& duration, const ActorIndex& actor);
	WaitObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { reset(area, actor); }
	void cancel(Area& area, const ActorIndex& actor) { reset(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "wait"; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canResume() const { return false; }
	friend class WaitScheduledEvent;
};
class WaitScheduledEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	WaitObjective& m_objective;
public:
	WaitScheduledEvent(const Step& delay, Area& area, WaitObjective& wo, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
};
