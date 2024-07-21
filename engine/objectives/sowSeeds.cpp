#include "sowSeeds.h"
#include "../area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../simulation.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "types.h"

struct DeserializationMemo;

SowSeedsEvent::SowSeedsEvent(Step delay, Area& area, SowSeedsObjective& o, ActorIndex actor, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_objective(o)
{ 
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
void SowSeedsEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	Blocks& blocks = area->getBlocks();
	BlockIndex block = m_objective.m_block;
	ActorIndex actor = m_actor.getIndex();
	FactionId faction = actors.getFactionId(actor);
	if(!blocks.farm_contains(block, faction))
	{
		// BlockIndex is no longer part of a field. It may have been undesignated or it may no longer be a suitable place to grow the selected plant.
		m_objective.reset(*area, actor);
		m_objective.execute(*area, actor);
	}
	assert(blocks.farm_get(block, faction));
	const PlantSpecies& plantSpecies = *blocks.farm_get(block, faction)->plantSpecies;
	blocks.plant_create(block, plantSpecies);
	actors.objective_complete(actor, m_objective);
}
void SowSeedsEvent::clearReferences(Simulation&, Area*){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return area.m_hasFarmFields.hasSowSeedsDesignations(area.getActors().getFactionId(actor));
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Area& area, ActorIndex) const
{
	return std::make_unique<SowSeedsObjective>(area);
}
SowSeedsObjective::SowSeedsObjective(Area& area) : 
	Objective(Config::sowSeedsPriority), 
	m_event(area.m_eventSchedule) { }
SowSeedsObjective::SowSeedsObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo), 
	m_event(deserializationMemo.m_simulation.m_eventSchedule), 
	m_block(data.contains("block") ? data["block"].get<BlockIndex>() : BLOCK_INDEX_MAX)
{
	if(data.contains("eventStart"))
		m_event.schedule(Config::sowSeedsStepsDuration, *this, data["eventStart"].get<Step>());
}
Json SowSeedsObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_block == BLOCK_INDEX_MAX)
		data["block"] = m_block;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
BlockIndex SowSeedsObjective::getBlockToSowAt(Area& area, BlockIndex location, Facing facing, ActorIndex actor)
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	std::function<bool(BlockIndex)> predicate = [&area, faction](BlockIndex block)
	{
		auto& blocks = area.getBlocks();
		return blocks.designation_has(block, faction, BlockDesignation::SowSeeds) && !blocks.isReserved(block, faction);
	};
	return actors.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
void SowSeedsObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	if(m_block != BLOCK_INDEX_MAX)
	{
		if(actors.isAdjacentToLocation(actor, m_block))
		{
			FarmField* field = blocks.farm_get(m_block, actors.getFactionId(actor));
			if(field != nullptr && blocks.plant_canGrowHereAtSomePointToday(m_block, *field->plantSpecies))
			{
				begin(area, actor);
				return;
			}
		}
		// Previously found m_block or path no longer valid, redo from start.
		reset(area, actor);
		execute(area, actor);
	}
	else
	{
		// Check if we can use an adjacent as m_block.
		if(actors.allOccupiedBlocksAreReservable(actor, actors.getFactionId(actor)))
		{
			BlockIndex block = getBlockToSowAt(area, actors.getLocation(actor), actors.getFacing(actor), actor);
			if(block != BLOCK_INDEX_MAX)
			{
				select(area, block, actor);
				bool result = actors.move_tryToReserveOccupied(actor);
				assert(result);
				begin(area, actor);
				return;
			}
		}
	}
	std::unique_ptr<PathRequest> pathRequest = std::make_unique<SowSeedsPathRequest>(area, *this);
	actors.move_pathRequestRecord(actor, std::move(pathRequest));
}
void SowSeedsObjective::cancel(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_event.maybeUnschedule();
	auto& blocks = area.getBlocks();
	FactionId faction = actors.getFactionId(actor);
	if(m_block != BLOCK_INDEX_MAX && blocks.farm_contains(m_block, faction))
	{
		FarmField* field = blocks.farm_get(m_block, faction);
		if(field == nullptr)
			return;
		//TODO: check it is still planting season.
		if(blocks.plant_canGrowHereAtSomePointToday(m_block, *field->plantSpecies))
			area.m_hasFarmFields.at(faction).addSowSeedsDesignation(m_block);
	}
}
void SowSeedsObjective::select(Area& area, BlockIndex block, ActorIndex actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	assert(!blocks.plant_exists(block));
	assert(blocks.farm_contains(block, actors.getFactionId(actor)));
	assert(m_block == BLOCK_INDEX_MAX);
	m_block = block;
	area.m_hasFarmFields.at(actors.getFactionId(actor)).removeSowSeedsDesignation(block);
}
void SowSeedsObjective::begin(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	assert(m_block != BLOCK_INDEX_MAX);
	assert(actors.isAdjacentToLocation(actor, m_block));
	m_event.schedule(Config::sowSeedsStepsDuration, area, *this);
}
void SowSeedsObjective::reset(Area& area, ActorIndex actor)
{
	cancel(area, actor);
	m_block = BLOCK_INDEX_MAX;
}
bool SowSeedsObjective::canSowAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	auto& blocks = area.getBlocks();
	return blocks.designation_has(block, faction, BlockDesignation::SowSeeds) && !blocks.isReserved(block, faction);
}
SowSeedsPathRequest::SowSeedsPathRequest(Area& area, SowSeedsObjective& objective) : ObjectivePathRequest(objective, true) // reserve block which passed predicate.
{
	ActorIndex actor = getActor();
	bool unreserved = true;
	createGoAdjacentToDesignation(area, actor, BlockDesignation::SowSeeds, objective.m_detour, unreserved, Config::maxRangeToSearchForHorticultureDesignations);
}
void SowSeedsPathRequest::onSuccess(Area& area, BlockIndex blockWhichPassedPredicate)
{
	static_cast<SowSeedsObjective&>(m_objective).select(area, blockWhichPassedPredicate, getActor());
}
