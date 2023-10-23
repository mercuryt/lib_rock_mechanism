#pragma once

#include "objective.h"
#include "threadedTask.hpp"
#include "findsPath.h"

class Actor;
class Block;
class WanderThreadedTask;

class WanderObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<WanderThreadedTask> m_threadedTask;
	bool m_routeFound;
public:
	WanderObjective(Actor& a);
	void execute();
	void cancel() { m_threadedTask.maybeCancel(); }
	void delay() { cancel(); }
	void reset();
	std::string name() const { return "wander"; }
	ObjectiveId getObjectiveId() const { return ObjectiveId::Wander; }
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
