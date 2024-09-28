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
WanderPathRequest::WanderPathRequest(Area& area, WanderObjective& objective, ActorIndex actor) : m_objective(objective)
{
	Random& random = area.m_simulation.m_random;
	m_blockCounter = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	DestinationCondition condition = [this](BlockIndex index, Facing) { return std::make_pair(!m_blockCounter--, index); };
	createGoToCondition(area, actor, condition, false, false, DistanceInBlocks::null());
}
WanderPathRequest::WanderPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_objective(static_cast<WanderObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{
	nlohmann::from_json(data, *this);
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
Json WanderPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_objective;
	return output;
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
	std::unique_ptr<PathRequest> m_pathRequest = std::make_unique<WanderPathRequest>(area, *this, actor);
	area.getActors().move_pathRequestRecord(actor, std::move(m_pathRequest));
}
void WanderObjective::cancel(Area& area, ActorIndex actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
bool WanderObjective::hasPathRequest(const Area& area, ActorIndex actor) const { return area.getActors().move_hasPathRequest(actor); }
void WanderObjective::reset(Area& area, ActorIndex actor) 
{ 
	cancel(area, actor); 
	area.getActors().canReserve_clearAll(actor);
}
