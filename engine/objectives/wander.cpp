#include "wander.h"
#include "wait.h"
#include "../actor.h"
#include "../deserializationMemo.h"
#include "../objective.h"
#include "../random.h"
#include "../config.h"
#include "../simulation.h"
WanderThreadedTask::WanderThreadedTask(WanderObjective& o) : 
	ThreadedTask(o.m_actor.getThreadedTaskEngine()), m_objective(o), m_findsPath(o.m_actor, false) { }
void WanderThreadedTask::readStep()
{
	BlockIndex lastBlock = BLOCK_INDEX_MAX;
	Random& random = m_objective.m_actor.getSimulation().m_random;
	uint32_t numberOfBlocks = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	std::function<bool(BlockIndex, Facing)> condition = [&](BlockIndex block, [[maybe_unused]] Facing facing)
	{
		lastBlock = block;
		return !numberOfBlocks--;
	};
	m_findsPath.pathToPredicate(condition);
	// If we do not have access to the number of blocks needed to pick a destination via numberOfBlocks then just path to the last block checked.
	if(!m_findsPath.found() && lastBlock != BLOCK_INDEX_MAX)
	{
		assert(m_objective.m_actor.m_location != lastBlock);
		m_findsPath.pathToBlock(lastBlock);
	}
}
void WanderThreadedTask::writeStep() 
{ 
	if(m_findsPath.found())
	{
		m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath()); 
		m_objective.m_started = true;
	}
	else
		m_objective.m_actor.wait(Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
}
void WanderThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
// Objective.
WanderObjective::WanderObjective(Actor& a) : Objective(a, 0u), m_threadedTask(a.getThreadedTaskEngine()) { }
WanderObjective::WanderObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json WanderObjective::toJson() const
{ 
	Json data = Objective::toJson();
	data["routeFound"] = m_started;
	return data;
}
void WanderObjective::execute() 
{ 
	if(m_started)
		m_actor.m_hasObjectives.objectiveComplete(*this);
	else
	{
		m_started = true;
		/*
		auto& random = m_actor.getSimulation().m_random;
		if(random.chance(Config::chanceToWaitInsteadOfWander))
		{
			Step duration = random.getInRange(Config::minimumDurationToWaitInsteadOfWander, Config::maximumDurationToWaitInsteadOfWander);
			m_actor.wait(duration);
		}
		else
		*/
		m_threadedTask.create(*this); 
	}
}

void WanderObjective::reset() 
{ 
	cancel(); 
	m_started = false; 
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
