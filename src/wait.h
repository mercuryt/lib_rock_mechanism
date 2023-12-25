#pragma once
#include "deserilizationMemo.h"
#include "eventSchedule.hpp"
#include "objective.h"
#include <string>

class WaitScheduledEvent;
class Actor;

class WaitObjective final : public Objective
{
	HasScheduledEvent<WaitScheduledEvent> m_event;
public:
	// Priority of waiting is 0.
	WaitObjective(Actor& a, Step duration);
	WaitObjective(const Json& data, DeserilizationMemo& deserilizationMemo);
	Json toJson() const;
	void execute();
	void delay() { reset(); }
	void cancel() { reset(); }
	void reset();
	std::string name() const { return "wait"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Wait; }
	friend class WaitScheduledEvent;
};
class WaitScheduledEvent final : public ScheduledEventWithPercent
{
	WaitObjective& m_objective;
public:
	WaitScheduledEvent(Step delay, WaitObjective& wo);
	void execute() { m_objective.execute(); }
	void clearReferences() { m_objective.m_event.clearPointer(); }
};
