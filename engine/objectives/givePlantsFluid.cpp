#include "givePlantsFluid.h"
#include "../area/area.h"
#include "../config.h"
#include "../itemType.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.h"
#include "../objective.h"
#include "../reservable.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../designations.h"
#include "../index.h"
#include "../items/items.h"
#include "../types.h"
#include "../plants.h"
#include "../path/terrainFacade.hpp"
#include "../hasShapes.hpp"

// Path Request.
GivePlantsFluidPathRequest::GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
GivePlantsFluidPathRequest::GivePlantsFluidPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<GivePlantsFluidObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{ }
FindPathResult GivePlantsFluidPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	assert(m_objective.m_project == nullptr);
	DistanceInBlocks maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	constexpr bool unreserved = false;
	return terrainFacade.findPathToBlockDesignation(memo, BlockDesignation::GivePlantFluid, faction, start, facing, shape, m_objective.m_detour, adjacent, unreserved, maxRange);
}
void GivePlantsFluidPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
		return;
	}
	if(area.m_blockDesignations.getForFaction(faction).check(result.blockThatPassedPredicate, BlockDesignation::GivePlantFluid))
		m_objective.selectPlantLocation(area, result.blockThatPassedPredicate, actorIndex);
	else
		actors.objective_canNotCompleteSubobjective(actorIndex);
}
Json GivePlantsFluidPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_objective;
	output["type"] = "give plants fluid";
	return output;
}
bool GivePlantsFluidObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	assert(actors.getLocation(actor).exists());
	// Pilots and passengers onDeck cannot give plants fluid.
	if(actors.onDeck_getIsOnDeckOf(actor).exists())
		return false;
	return area.m_hasFarmFields.hasGivePlantsFluidDesignations(actors.getFaction(actor));
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	return std::make_unique<GivePlantsFluidObjective>();
}
// Objective
GivePlantsFluidObjective::GivePlantsFluidObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{
	if(data.contains("project"))
		m_project = std::make_unique<GivePlantFluidProject>(data["project"], deserializationMemo, area);
}
Json GivePlantsFluidObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project->toJson();
	return data;
}
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute(Area& area, const ActorIndex& actor)
{
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
		makePathRequest(area, actor);
}
void GivePlantsFluidObjective::cancel(Area&, const ActorIndex&)
{
	if(m_project != nullptr)
	{
		m_project->cancel();
		m_project = nullptr;
	}
}
void GivePlantsFluidObjective::selectPlantLocation(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	m_project = std::make_unique<GivePlantFluidProject>(block, area, faction);
	m_project->addWorkerCandidate(actor, *this);
}
void GivePlantsFluidObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
}
void GivePlantsFluidObjective::delay(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
}
void GivePlantsFluidObjective::makePathRequest(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<GivePlantsFluidPathRequest>(area, *this, actor));
}
void GivePlantsFluidObjective::onBeforeUnload(Area&, const ActorIndex&)
{
	if(m_project != nullptr)
		m_project->clearReservations();
}