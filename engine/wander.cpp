#include "wander.h"
#include "actor.h"
#include "block.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "random.h"
#include "config.h"
#include "simulation.h"
#include "wait.h"
WanderThreadedTask::WanderThreadedTask(WanderObjective& o) : ThreadedTask(o.m_actor.getThreadedTaskEngine()), m_objective(o), m_findsPath(o.m_actor, false) { }
void WanderThreadedTask::readStep()
{
	const Block* lastBlock = nullptr;
	Random& random = m_objective.m_actor.getSimulation().m_random;
	uint32_t numberOfBlocks = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	std::function<bool(const Block&, Facing)> condition = [&](const Block& block, [[maybe_unused]] Facing facing)
	{
		lastBlock = &block;
		return !numberOfBlocks--;
	};
	m_findsPath.pathToPredicate(condition);
	if(!m_findsPath.found() && lastBlock != nullptr)
	{
		assert(m_objective.m_actor.m_location != lastBlock);
		m_findsPath.pathToBlock(*lastBlock);
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
	m_findsPath.cacheMoveCosts();
}
void WanderThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
// Objective.
WanderObjective::WanderObjective(Actor& a) : Objective(a, 0u), m_threadedTask(a.getThreadedTaskEngine()), m_routeFound(false) { }
WanderObjective::WanderObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), m_routeFound(false)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json WanderObjective::toJson() const
{ 
	Json data = Objective::toJson();
	data["routeFound"] = m_routeFound;
	return data;
}
void WanderObjective::execute() 
{ 
	if(m_routeFound)
		m_actor.m_hasObjectives.objectiveComplete(*this);
	else
	{
		auto& random = m_actor.getSimulation().m_random;
		if(random.chance(Config::chanceToWaitInsteadOfWander))
		{
			Step duration = random.getInRange(Config::minimumDurationToWaitInsteadOfWander, Config::maximumDurationToWaitInsteadOfWander);
			std::unique_ptr<Objective> waitObjective = std::make_unique<WaitObjective>(m_actor, duration);
			m_actor.m_hasObjectives.addTaskToStart(std::move(waitObjective));
		}
		else
		{
			m_threadedTask.create(*this); 
			m_routeFound = true;
		}
	}
}

void WanderObjective::reset() 
{ 
	cancel(); 
	m_routeFound = false; 
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
