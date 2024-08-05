#include "wander.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "../random.h"
#include "../config.h"
#include "../simulation.h"
#include "actors/actors.h"
#include "types.h"
#include <memory>
// PathRequest
WanderPathRequest::WanderPathRequest(Area& area, WanderObjective& objective) : m_objective(objective)
{
	Random& random = area.m_simulation.m_random;
	m_blockCounter = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	std::function<bool(BlockIndex, Facing)> condition = [this](BlockIndex, Facing) { return !m_blockCounter--; };
	createGoToCondition(area, getActor(), condition, false, false, DistanceInBlocks::null());
}
void WanderPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty())
		actors.wait(actor, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
	else
		actors.move_setPath(actor, result.path);
}
// Objective.
WanderObjective::WanderObjective() : Objective(Priority::create(0)) { }
WanderObjective::WanderObjective(const Json& data) : Objective(data) { }
Json WanderObjective::toJson() const
{ 
	Json data = Objective::toJson();
	return data;
}
void WanderObjective::execute(Area& area, ActorIndex actor) 
{ 
	std::unique_ptr<PathRequest> m_pathRequest = std::make_unique<WanderPathRequest>(area, *this);
	area.getActors().move_pathRequestRecord(actor, std::move(m_pathRequest));
}
void WanderObjective::cancel(Area& area, ActorIndex actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
bool WanderObjective::hasPathRequest(const Area& area, ActorIndex actor) const { return area.getActors().move_hasPathRequest(actor); }
void WanderObjective::reset(Area& area, ActorIndex actor) 
{ 
	cancel(area, actor); 
	area.getActors().canReserve_clearAll(actor);
}
