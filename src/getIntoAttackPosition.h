#pragma once

#include "threadedTask.h"
#include "path.h"
#include "actor.h"

class GetIntoAttackPositionThreadedTask : ThreadedTask
{
	Actor& m_actor;
	Actor& m_target;
	uint32_t m_range;
	std::vector<Block*> m_route;
	GetIntoAttackPositionThreadedTask(Actor& a, Actor& t, uint32_t r) : m_actor(a), m_target(t), m_range(r) {}
	void readStep();
	void writeStep();
};
