#pragma once
#include "eventSchedule.hpp"
#include "objective.h"
#include <string>

class WaitScheduledEvent;
class Actor;

class WaitObjective final : public Objective
{
	Actor& m_actor;
	HasScheduledEvent<WaitScheduledEvent> m_event;
public:
	// Priority of waiting is 0.
	WaitObjective(Actor& a, Step duration);
	void execute();
	void delay() { reset(); }
	void cancel() { reset(); }
	void reset();
	std::string name() const { return "wait"; }
	ObjectiveId getObjectiveId() const { return ObjectiveId::Wait; }
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
