#include "wander.h"
#include "actor.h"
#include "block.h"
#include "path.h"
#include "randomUtil.h"
#include "config.h"
WanderThreadedTask::WanderThreadedTask(WanderObjective& o) : ThreadedTask(o.m_actor.getThreadedTaskEngine()), m_objective(o) { }
void WanderThreadedTask::readStep()
{
	auto condition = [&](const Block& block)
	{
		return randomUtil::percentChance(block.taxiDistance(*m_objective.m_actor.m_location) * Config::wanderDistanceModifier);
	};
	m_route = path::getForActorToPredicate(m_objective.m_actor, condition);
}
void WanderThreadedTask::writeStep() { m_objective.m_actor.m_canMove.setPath(m_route); }
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
