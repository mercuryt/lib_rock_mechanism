#pragma once
#include "findsPath.h"
#include "objective.h"
#include "config.h"
#include "threadedTask.hpp"
struct DeserializationMemo;
class LeaveAreaThreadedTask;
class LeaveAreaObjective final : public Objective
{
public:
	HasThreadedTask<LeaveAreaThreadedTask> m_task;
	LeaveAreaObjective(ActorIndex a, uint8_t priority);
	// No need to overide default to/from json.
	void execute();
	void cancel() { }
	void delay() { }
	void reset(Area&) { }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::LeaveArea; }
	[[nodiscard]] std::string name() const { return "leave area"; }
	[[nodiscard]] bool actorIsAdjacentToEdge() const;
};
class LeaveAreaThreadedTask final : public ThreadedTask
{
	LeaveAreaObjective& m_objective;
	FindsPath m_findsPath;
public:
	LeaveAreaThreadedTask(LeaveAreaObjective& objective);
	void readStep(Simulation& simulation, Area* area);
	void writeStep(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { m_objective.m_task.clearPointer(); }
};
