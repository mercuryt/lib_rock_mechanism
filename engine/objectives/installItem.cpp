#include "installItem.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../items/items.h"
#include "../types.h"
#include "../terrainFacade.hpp"
// PathRequest.
InstallItemPathRequest::InstallItemPathRequest(Area& area, InstallItemObjective& iio, const ActorIndex& actorIndex) :
	m_installItemObjective(iio)
{ 
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_installItemObjective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
InstallItemPathRequest::InstallItemPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestDepthFirst(data, area),
	m_installItemObjective(static_cast<InstallItemObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
Json InstallItemPathRequest::toJson() const
{
	Json output = static_cast<const PathRequestDepthFirst&>(*this);
	output["objective"] = reinterpret_cast<uintptr_t>(&m_installItemObjective);
	output["type"] = "install item";
	return output;
}
FindPathResult InstallItemPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	auto destinationCondition = [&, actorIndex](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
	{
		FactionId faction = actors.getFactionId(actorIndex);
		return {area.m_hasInstallItemDesignations.getForFaction(faction).contains(block), block};
	};
	constexpr bool adjacent = true;
	constexpr bool useAnyBlock = true;
	const BlockIndex& huristicDestination = m_installItemObjective.m_project->getLocation();
	return terrainFacade.findPathToConditionDepthFirst<useAnyBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, huristicDestination, m_installItemObjective.m_detour, adjacent);
}
void InstallItemPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actorIndex, m_installItemObjective);
	else
	{
		BlockIndex block = result.blockThatPassedPredicate;
		auto& hasInstallItemDesignations = area.m_hasInstallItemDesignations.getForFaction(actors.getFactionId(actorIndex));
		if(!hasInstallItemDesignations.contains(block) || !hasInstallItemDesignations.getForBlock(block).canAddWorker(actorIndex))
			// Canceled or reserved, try again.
			m_installItemObjective.execute(area, actorIndex);
		else
			hasInstallItemDesignations.getForBlock(block).addWorkerCandidate(actorIndex, m_installItemObjective);
	}

}
// Objective.
InstallItemObjective::InstallItemObjective() : Objective(Config::installItemPriority) { }
InstallItemObjective::InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo)
{ 
	if(data.contains("project"))
		m_project = static_cast<InstallItemProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()));
}
Json InstallItemObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project)
		data["project"] = m_project;
	return data;
}
void InstallItemObjective::execute(Area& area, const ActorIndex& actor)
{
	if(m_project)
		m_project->commandWorker(actor);
	else
		area.getActors().move_pathRequestRecord(actor, std::make_unique<InstallItemPathRequest>(area, *this, actor));
}
void InstallItemObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().move_pathRequestMaybeCancel(actor); m_project->removeWorker(actor); }
bool InstallItemObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	return !area.m_hasInstallItemDesignations.getForFaction(area.getActors().getFactionId(actor)).empty();
}
std::unique_ptr<Objective> InstallItemObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjective>();
	return objective;
}
void InstallItemObjective::reset(Area& area, const ActorIndex& actor) { area.getActors().canReserve_clearAll(actor); m_project = nullptr; }
