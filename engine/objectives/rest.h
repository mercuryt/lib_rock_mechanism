#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "config.h"

class Simulation;
class RestEvent;
struct DeserializationMemo;

class RestObjective final : public Objective
{
	HasScheduledEvent<RestEvent> m_restEvent;
public:
	RestObjective(Area& area, ActorIndex a);
	RestObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void cancel(Area&) { m_restEvent.maybeUnschedule(); }
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	std::string name() const { return "rest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Rest; }
	friend class RestEvent;
};
class RestEvent final : public ScheduledEvent
{
	RestObjective& m_objective;
public:
	RestEvent(Simulation& simulation, RestObjective& ro, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_restEvent.clearPointer(); }
};
