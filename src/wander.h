#pragma once

#include "objective.h"
#include "threadedTask.h"

class Actor;
class Block;
class WanderObjective;

class WanderThreadedTask final : public ThreadedTask
{
	WanderObjective& m_objective;
	std::vector<Block*> m_route;
public:
	WanderThreadedTask(WanderObjective& o) : m_objective(o) { }
	void readStep();
	void writeStep();
	void clearReferences();
};

class WanderObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<WanderThreadedTask> m_threadedTask;
	bool m_routeFound;
public:
	WanderObjective(Actor& a);
	void execute();
	void cancel() { }
	friend class WanderThreadedTask;
};
