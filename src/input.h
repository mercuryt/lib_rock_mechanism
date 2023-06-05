#pragma once

#include "objective.h"
#include "queuedAction.h"
#include <vector>
#include <memory>

template<class Actor>
class InputObjective : Objective
{
	Actor& m_actor;
	std::vector<std::unique_ptr<QueuedAction>> m_actions;
	InputObjective(Actor& a) : Objective(Config::inputObjectivePriority), m_actor(a) {}

	void execute()
	{
		assert(m_actor.m_actionQueue.empty());
		for(std::unique_ptr<QueuedAction>& action : m_actions)
			m_actor.m_actionQueue.push_back(std::move(action));
	}
}
