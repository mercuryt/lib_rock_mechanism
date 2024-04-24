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
	RestObjective(Actor& a);
	RestObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute() { m_restEvent.schedule(*this); }
	void cancel() { m_restEvent.maybeUnschedule(); }
	void delay() { cancel(); }
	void reset();
	std::string name() const { return "rest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Rest; }
	friend class RestEvent;
};
class RestEvent final : public ScheduledEvent
{
	RestObjective& m_objective;
public:
	RestEvent(RestObjective& ro, const Step start = 0);
	void execute();
	void clearReferences() { m_objective.m_restEvent.clearPointer(); }
};
