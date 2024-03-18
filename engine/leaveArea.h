#pragma once
#include "deserializationMemo.h"
#include "findsPath.h"
#include "objective.h"
#include "config.h"
#include "threadedTask.h"
#include "threadedTask.hpp"
struct DeserializationMemo;
class Block;
class Actor;
class LeaveAreaThreadedTask;
class LeaveAreaObjective final : public Objective
{
public:
	HasThreadedTask<LeaveAreaThreadedTask> m_task;
	LeaveAreaObjective(Actor& a, uint8_t priority);
	// No need to overide default to/from json.
	void execute();
	void cancel() { }
	void delay() { }
	void reset() { }
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
	void readStep();
	void writeStep();
	void clearReferences() { m_objective.m_task.clearPointer(); }
};
