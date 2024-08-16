#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "config.h"
#include "types.h"

class Simulation;
class RestEvent;
struct DeserializationMemo;

class RestObjective final : public Objective
{
	HasScheduledEvent<RestEvent> m_restEvent;
public:
	RestObjective(Area& area);
	RestObjective(const Json& data, Area& area, ActorIndex actor);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area&, ActorIndex) { m_restEvent.maybeUnschedule(); }
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "rest"; }
friend class RestEvent;
};
class RestEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	RestObjective& m_objective;
public:
	RestEvent(Area& area, RestObjective& ro, ActorIndex actor, Step start = Step::create(0));
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_restEvent.clearPointer(); }
};
