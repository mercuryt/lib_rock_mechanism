#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "config.h"

class RestEvent;
class Actor;
struct DeserializationMemo;

class RestObjective final : public Objective
{
	HasScheduledEvent<RestEvent> m_restEvent;
public:
	RestObjective(ActorIndex a);
	RestObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute() { m_restEvent.schedule(*this); }
	void cancel() { m_restEvent.maybeUnschedule(); }
	void delay() { cancel(); }
	void reset(Area& area);
	std::string name() const { return "rest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Rest; }
	friend class RestEvent;
};
class RestEvent final : public ScheduledEvent
{
	RestObjective& m_objective;
public:
	RestEvent(RestObjective& ro, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_restEvent.clearPointer(); }
};
