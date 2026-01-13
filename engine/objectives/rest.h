#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "config/config.h"
#include "numericTypes/types.h"

class Simulation;
class RestEvent;
struct DeserializationMemo;

class RestObjective final : public Objective
{
	HasScheduledEvent<RestEvent> m_restEvent;
public:
	RestObjective(Area& area);
	RestObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex&) { m_restEvent.maybeUnschedule(); }
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "rest"; }
friend class RestEvent;
};
class RestEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	RestObjective& m_objective;
public:
	RestEvent(Area& area, RestObjective& ro, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_restEvent.clearPointer(); }
};
