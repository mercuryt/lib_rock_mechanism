#pragma once

#include "deserializationMemo.h"
#include "objective.h"
#include "threadedTask.hpp"
#include "findsPath.h"

class Actor;
class Block;
class WanderThreadedTask;

class WanderObjective final : public Objective
{
	HasThreadedTask<WanderThreadedTask> m_threadedTask;
	bool m_started = false;
public:
	WanderObjective(Actor& a);
	WanderObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel() { m_threadedTask.maybeCancel(); }
	void delay() { cancel(); }
	void reset();
	std::string name() const { return "wander"; }
	[[nodiscard]] bool canResume() const { return false; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Wander; }
	friend class WanderThreadedTask;
};
class WanderThreadedTask final : public ThreadedTask
{
	WanderObjective& m_objective;
	FindsPath m_findsPath;
public:
	WanderThreadedTask(WanderObjective& o);
	void readStep();
	void writeStep();
	void clearReferences();
};
