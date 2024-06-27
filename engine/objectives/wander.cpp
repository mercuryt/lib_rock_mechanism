#include "wander.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "../random.h"
#include "../config.h"
#include "../simulation.h"
#include <memory>
// PathRequest
WanderPathRequest::WanderPathRequest(Area& area, WanderObjective& objective) : m_objective(objective)
{
	Random& random = area.m_simulation.m_random;
	m_blockCounter = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	std::function<bool(BlockIndex)> condition = [this](BlockIndex) { return !m_blockCounter--; };
	createGoToCondition(area, m_objective.m_actor, condition, false, false, BLOCK_DISTANCE_MAX);
}
void WanderPathRequest::callback(Area& area, FindPathResult result)
{
	if(result.path.empty())
		area.m_actors.wait(m_objective.m_actor, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
	else
		area.m_actors.move_setPath(m_objective.m_actor, result.path);
}
// Objective.
WanderObjective::WanderObjective(ActorIndex a) : Objective(a, 0u) { }
/*
WanderObjective::WanderObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json WanderObjective::toJson() const
{ 
	Json data = Objective::toJson();
	return data;
}
*/
void WanderObjective::execute(Area& area) 
{ 
	std::unique_ptr<PathRequest> m_pathRequest = std::make_unique<WanderPathRequest>(area, *this);
	area.m_actors.move_setDestinationWithPathRequest(std::move(m_pathRequest));
}
void WanderObjective::reset(Area& area) 
{ 
	cancel(area); 
	area.m_actors.canReserve_clearAll(m_actor);
}
