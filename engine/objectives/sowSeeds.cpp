#include "sowSeeds.h"
#include "../area/area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../simulation/simulation.h"
#include "../hasShapes.hpp"
#include "../path/terrainFacade.hpp"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../designations.h"
#include "../numericTypes/types.h"

struct DeserializationMemo;

SowSeedsEvent::SowSeedsEvent(const Step& delay, Area& area, SowSeedsObjective& o, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_objective(o)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void SowSeedsEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	Blocks& blocks = area->getBlocks();
	BlockIndex block = m_objective.m_block;
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	FactionId faction = actors.getFaction(actor);
	if(!blocks.farm_contains(block, faction))
	{
		// BlockIndex is no longer part of a field. It may have been undesignated or it may no longer be a suitable place to grow the selected plant.
		m_objective.reset(*area, actor);
		m_objective.execute(*area, actor);
	}
	assert(blocks.farm_get(block, faction));
	PlantSpeciesId plantSpecies = blocks.farm_get(block, faction)->plantSpecies;
	blocks.plant_create(block, plantSpecies, Percent::create(0));
	actors.objective_complete(actor, m_objective);
}
void SowSeedsEvent::clearReferences(Simulation&, Area*){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot sow.
	if(actors.onDeck_getIsOnDeckOf(actor).exists())
		return false;
	return area.m_hasFarmFields.hasSowSeedsDesignations(area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Area& area, const ActorIndex&) const
{
	return std::make_unique<SowSeedsObjective>(area);
}
SowSeedsObjective::SowSeedsObjective(Area& area) :
	Objective(Config::sowSeedsPriority),
	m_event(area.m_eventSchedule) { }
SowSeedsObjective::SowSeedsObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_event(area.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_event.schedule(Config::sowSeedsStepsDuration, area, *this, actor, data["eventStart"].get<Step>());
	if(data.contains("block"))
		m_block = data["block"].get<BlockIndex>();
}
Json SowSeedsObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_block.exists())
		data["block"] = m_block;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
BlockIndex SowSeedsObjective::getBlockToSowAt(Area& area, const BlockIndex& location, Facing4 facing, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFaction(actor);
	AreaHasBlockDesignationsForFaction& designations = area.m_blockDesignations.getForFaction(faction);
	Blocks& blocks = area.getBlocks();
	auto offset = designations.getOffsetForDesignation(BlockDesignation::SowSeeds);
	std::function<bool(const BlockIndex&)> predicate = [&, offset, faction](const BlockIndex& block)
	{
		return designations.checkWithOffset(offset, block) && !blocks.isReserved(block, faction);
	};
	return actors.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
void SowSeedsObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	if(m_block.exists())
	{
		if(actors.isAdjacentToLocation(actor, m_block))
		{
			FarmField* field = blocks.farm_get(m_block, actors.getFaction(actor));
			if(field != nullptr && blocks.plant_canGrowHereAtSomePointToday(m_block, field->plantSpecies))
			{
				begin(area, actor);
				return;
			}
		}
		// Previously found m_block or path no longer valid, redo from start.
		reset(area, actor);
		execute(area, actor);
		return;
	}
	else
	{
		// Check if we can use an adjacent as m_block.
		if(actors.allOccupiedBlocksAreReservable(actor, actors.getFaction(actor)))
		{
			BlockIndex block = getBlockToSowAt(area, actors.getLocation(actor), actors.getFacing(actor), actor);
			if(block.exists())
			{
				select(area, block, actor);
				assert(actors.move_tryToReserveOccupied(actor));
				begin(area, actor);
				return;
			}
		}
	}
	actors.move_pathRequestRecord(actor, std::make_unique<SowSeedsPathRequest>(area, *this, actor));
}
void SowSeedsObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_event.maybeUnschedule();
	auto& blocks = area.getBlocks();
	FactionId faction = actors.getFaction(actor);
	if(m_block.exists() && blocks.farm_contains(m_block, faction))
	{
		FarmField* field = blocks.farm_get(m_block, faction);
		if(field == nullptr)
			return;
		//TODO: check it is still planting season.
		if(blocks.plant_canGrowHereAtSomePointToday(m_block, field->plantSpecies))
			area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(area, m_block);
	}
}
void SowSeedsObjective::select(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	[[maybe_unused]] Blocks& blocks = area.getBlocks();
	[[maybe_unused]] Actors& actors = area.getActors();
	assert(!blocks.plant_exists(block));
	assert(blocks.farm_contains(block, actors.getFaction(actor)));
	assert(m_block.empty());
	m_block = block;
	area.m_hasFarmFields.getForFaction(actors.getFaction(actor)).removeSowSeedsDesignation(area, block);
}
void SowSeedsObjective::begin(Area& area, const ActorIndex& actor)
{
	[[maybe_unused]] Actors& actors = area.getActors();
	assert(m_block.exists());
	assert(actors.isAdjacentToLocation(actor, m_block));
	m_event.schedule(Config::sowSeedsStepsDuration, area, *this, actor);
}
void SowSeedsObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_block.clear();
}
bool SowSeedsObjective::canSowAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFaction(actor);
	auto& blocks = area.getBlocks();
	return blocks.designation_has(block, faction, BlockDesignation::SowSeeds) && !blocks.isReserved(block, faction);
}
SowSeedsPathRequest::SowSeedsPathRequest(Area& area, SowSeedsObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
SowSeedsPathRequest::SowSeedsPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<SowSeedsObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
FindPathResult SowSeedsPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	constexpr bool unreserved = false;
	return terrainFacade.findPathToBlockDesignation(memo, BlockDesignation::SowSeeds, actors.getFaction(actorIndex), actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_objective.m_detour, adjacent, unreserved, Config::maxRangeToSearchForHorticultureDesignations);
}
void SowSeedsPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	const BlockIndex& sowLocation = result.blockThatPassedPredicate;
	if(result.path.empty() && sowLocation.empty())
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
	else if(!area.m_blockDesignations.getForFaction(faction).check(sowLocation, BlockDesignation::SowSeeds))
		// Retry. The location is no longer designated.
		actors.objective_canNotCompleteSubobjective(actorIndex);
	else
	{
		m_objective.select(area, result.blockThatPassedPredicate, actorIndex);
		actors.move_setPath(actorIndex, result.path);
	}
}
Json SowSeedsPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = name();
	return output;
}