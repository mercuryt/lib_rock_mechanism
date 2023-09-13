#include "wander.h"
#include "actor.h"
#include "block.h"
#include "path.h"
#include "randomUtil.h"
#include "config.h"
WanderThreadedTask::WanderThreadedTask(WanderObjective& o) : ThreadedTask(o.m_actor.getThreadedTaskEngine()), m_objective(o) { }
void WanderThreadedTask::readStep()
{
	const Block* lastBlock = nullptr;
	auto condition = [&](const Block& block)
	{
		lastBlock = &block;
		return randomUtil::percentChance(block.taxiDistance(*m_objective.m_actor.m_location) * Config::wanderDistanceModifier);
	};
	m_findsPath.pathToPredicate(m_objective.m_actor, condition);
	if(!m_findsPath.found() && lastBlock != nullptr)
	{
		assert(m_objective.m_actor.m_location != lastBlock);
		m_findsPath.pathToBlock(m_objective.m_actor, *lastBlock);
	}
}
void WanderThreadedTask::writeStep() 
{ 
	if(m_findsPath.found())
	{
		m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath()); 
		m_objective.m_routeFound = true;
	}
	else
		m_objective.m_actor.wait(Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
	m_findsPath.cacheMoveCosts(m_objective.m_actor);
}
void WanderThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
WanderObjective::WanderObjective(Actor& a) : Objective(0), m_actor(a), m_threadedTask(a.getThreadedTaskEngine()), m_routeFound(false) { }
void WanderObjective::execute() 
{ 
	if(m_routeFound)
		m_actor.m_hasObjectives.objectiveComplete(*this);
	else
	{
		m_threadedTask.create(*this); 
		m_routeFound = true;
	}
}
