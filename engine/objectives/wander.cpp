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
WanderPathRequest::WanderPathRequest(Area& area, WanderObjective& objective, const ActorIndex& actor) : m_objective(objective)
{
	Random& random = area.m_simulation.m_random;
	m_blockCounter = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	DestinationCondition condition = [this](BlockIndex index, Facing){
		if(!m_blockCounter)
			return std::pair(true, index);
		m_lastBlock = index;
		return std::pair(false, BlockIndex::null());
	};
	bool detour = false;
	bool unreserved = false;
	createGoToCondition(area, actor, condition, detour, unreserved, DistanceInBlocks::max());
}
WanderPathRequest::WanderPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_objective(static_cast<WanderObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))),
	m_lastBlock(data["lastBlock"].get<BlockIndex>())
{
	nlohmann::from_json(data, *this);
}
void WanderPathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty())
	{
		if(m_lastBlock.empty())
			actors.wait(actor, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
		else
		{
			m_objective.m_destination = m_lastBlock;
			actors.move_setDestination(actor, m_objective.m_destination);
		}
	}
	else
	{
		m_objective.m_destination = result.path.back();
		actors.move_setPath(actor, result.path);
	}
}
Json WanderPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_objective;
	output["lastBlock"] = m_lastBlock;
	return output;
}
// Objective.
WanderObjective::WanderObjective() : Objective(Priority::create(0)) { }
WanderObjective::WanderObjective(const Json& data) : Objective(data) { }
Json WanderObjective::toJson() const
{ 
	Json data = Objective::toJson();
	if(m_destination.exists())
		data["destination"] = m_destination;
	return data;
}
void WanderObjective::execute(Area& area, const ActorIndex& actor) 
{ 
	Actors& actors = area.getActors();
	if(m_destination.exists())
	{
		if(actors.getLocation(actor) == m_destination)
			actors.objective_complete(actor, *this);
		else
			actors.move_setDestination(actor, m_destination);
	}
	else
	{
		std::unique_ptr<PathRequest> pathRequest = std::make_unique<WanderPathRequest>(area, *this, actor);
		area.getActors().move_pathRequestRecord(actor, std::move(pathRequest));
	}
}
void WanderObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
bool WanderObjective::hasPathRequest(const Area& area, const ActorIndex& actor) const { return area.getActors().move_hasPathRequest(actor); }
void WanderObjective::reset(Area& area, const ActorIndex& actor) 
{ 
	cancel(area, actor); 
	area.getActors().canReserve_clearAll(actor);
}
