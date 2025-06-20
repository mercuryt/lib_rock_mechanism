#include "wander.h"
#include "../area/area.h"
#include "../deserializationMemo.h"
#include "../random.h"
#include "../config.h"
#include "../simulation/simulation.h"
#include "../path/terrainFacade.hpp"
#include "../actors/actors.h"
#include "../numericTypes/types.h"
// PathRequest
WanderPathRequest::WanderPathRequest(Area& area, WanderObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
}
WanderPathRequest::WanderPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<WanderObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))),
	m_lastBlock(data["lastBlock"].get<BlockIndex>())
{ }
FindPathResult WanderPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Random& random = area.m_simulation.m_random;
	m_blockCounter = random.getInRange(Config::wanderMinimimNumberOfBlocks, Config::wanderMaximumNumberOfBlocks);
	auto destinationCondition = [this](const BlockIndex& block, const Facing4&) -> std::pair<bool, BlockIndex>
	{
		if(!m_blockCounter)
			return std::pair(true, block);
		m_lastBlock = block;
		return std::pair(false, BlockIndex::null());
	};
	Actors& actors = area.getActors();
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	constexpr bool anyOccupiedBlock = false;
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupiedBlock>(destinationCondition, memo, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), detour, adjacent);
}
void WanderPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty())
	{
		if(m_lastBlock.empty())
			actors.wait(actorIndex, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
		else
		{
			m_objective.m_destination = m_lastBlock;
			actors.move_setDestination(actorIndex, m_objective.m_destination);
		}
	}
	else
	{
		m_objective.m_destination = result.path.back();
		actors.move_setPath(actorIndex, result.path);
	}
}
Json WanderPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_objective;
	output["lastBlock"] = m_lastBlock;
	output["type"] = "wander";
	return output;
}
// Objective.
WanderObjective::WanderObjective() : Objective(Priority::create(0)) { }
WanderObjective::WanderObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo) { }
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
		area.getActors().move_pathRequestRecord(actor, std::make_unique<WanderPathRequest>(area, *this, actor));
}
void WanderObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
bool WanderObjective::hasPathRequest(const Area& area, const ActorIndex& actor) const { return area.getActors().move_hasPathRequest(actor); }
void WanderObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	area.getActors().canReserve_clearAll(actor);
}
