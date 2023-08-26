#pragma once

#include "objective.h"
#include "threadedTask.hpp"

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
	void cancel() { }
	std::string name() { return "wander"; }
	friend class WanderThreadedTask;
};
class WanderThreadedTask final : public ThreadedTask
{
	WanderObjective& m_objective;
	std::vector<Block*> m_route;
public:
	WanderThreadedTask(WanderObjective& o);
	void readStep();
	void writeStep();
	void clearReferences();
};
