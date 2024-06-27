#include "sowSeeds.h"
#include "../area.h"
#include "../actor.h"
#include "../config.h"
#include "../farmFields.h"
#include "../simulation.h"
#include "types.h"

struct DeserializationMemo;

SowSeedsEvent::SowSeedsEvent(Area& area, Step delay, SowSeedsObjective& o, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_objective(o) { }
void SowSeedsEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->m_actors;
	Blocks& blocks = area->getBlocks();
	BlockIndex block = m_objective.m_block;
	Faction& faction = *actors.getFaction(m_objective.m_actor);
	if(!blocks.farm_contains(block, faction))
	{
		// BlockIndex is no longer part of a field. It may have been undesignated or it may no longer be a suitable place to grow the selected plant.
		m_objective.reset(*area);
		m_objective.execute(*area);
	}
	assert(blocks.farm_get(block, faction));
	const PlantSpecies& plantSpecies = *blocks.farm_get(block, faction)->plantSpecies;
	blocks.plant_create(block, plantSpecies);
	actors.objective_complete(m_objective.m_actor, m_objective);
}
void SowSeedsEvent::clearReferences(Simulation&, Area*){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return area.m_hasFarmFields.hasSowSeedsDesignations(*area.m_actors.getFaction(actor));
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Area& area, ActorIndex actor) const
{
	return std::make_unique<SowSeedsObjective>(area, actor);
}
SowSeedsObjective::SowSeedsObjective(Area& area, ActorIndex a) : 
	Objective(a, Config::sowSeedsPriority), 
	m_event(area.m_eventSchedule) { }
/*
SowSeedsObjective::SowSeedsObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo), 
	m_event(deserializationMemo.m_simulation.m_eventSchedule), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_block(data.contains("block") ? data["block"].get<BlockIndex>() : BLOCK_INDEX_MAX)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_event.schedule(Config::sowSeedsStepsDuration, *this, data["eventStart"].get<Step>());
}
Json SowSeedsObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_block == BLOCK_INDEX_MAX)
		data["block"] = m_block;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
*/
BlockIndex SowSeedsObjective::getBlockToSowAt(Area& area, BlockIndex location, Facing facing)
{
	Faction* faction = area.m_actors.getFaction(m_actor);
	std::function<bool(BlockIndex)> predicate = [&area, faction](BlockIndex block)
	{
		auto& blocks = area.getBlocks();
		return blocks.designation_has(block, *faction, BlockDesignation::SowSeeds) && !blocks.isReserved(block, *faction);
	};
	return area.m_actors.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(m_actor, location, facing, predicate);
}
void SowSeedsObjective::execute(Area& area)
{
	Actors& actors = area.m_actors;
	Blocks& blocks = area.getBlocks();
	if(m_block != BLOCK_INDEX_MAX)
	{
		if(actors.isAdjacentToLocation(m_actor, m_block))
		{
			FarmField* field = blocks.farm_get(m_block, *actors.getFaction(m_actor));
			if(field != nullptr && blocks.plant_canGrowHereAtSomePointToday(m_block, *field->plantSpecies))
			{
				begin(area);
				return;
			}
		}
		// Previously found m_block or path no longer valid, redo from start.
		reset(area);
		execute(area);
	}
	else
	{
		// Check if we can use an adjacent as m_block.
		if(actors.allOccupiedBlocksAreReservable(m_actor, *actors.getFaction(m_actor)))
		{
			BlockIndex block = getBlockToSowAt(area, actors.getLocation(m_actor), actors.getFacing(m_actor));
			if(block != BLOCK_INDEX_MAX)
			{
				select(area, block);
				bool result = actors.move_tryToReserveOccupied(m_actor);
				assert(result);
				begin(area);
				return;
			}
		}
	}
	std::unique_ptr<PathRequest> pathRequest = std::make_unique<SowSeedsPathRequest>(area, *this);
	actors.move_pathRequestRecord(m_actor, std::move(pathRequest));
}
void SowSeedsObjective::cancel(Area& area)
{
	Actors& actors = area.m_actors;
	actors.canReserve_clearAll(m_actor);
	actors.move_pathRequestMaybeCancel(m_actor);
	m_event.maybeUnschedule();
	auto& blocks = area.getBlocks();
	Faction& faction = *area.m_actors.getFaction(m_actor);
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
void SowSeedsObjective::select(Area& area, BlockIndex block)
{
	auto& blocks = area.getBlocks();
	assert(!blocks.plant_exists(block));
	assert(blocks.farm_contains(block, *area.m_actors.getFaction(m_actor)));
	assert(m_block == BLOCK_INDEX_MAX);
	m_block = block;
	area.m_hasFarmFields.at(*area.m_actors.getFaction(m_actor)).removeSowSeedsDesignation(block);
}
void SowSeedsObjective::begin(Area& area)
{
	assert(m_block != BLOCK_INDEX_MAX);
	assert(area.m_actors.isAdjacentToLocation(m_actor, m_block));
	m_event.schedule(Config::sowSeedsStepsDuration, *this);
}
void SowSeedsObjective::reset(Area& area)
{
	cancel(area);
	m_block = BLOCK_INDEX_MAX;
}
bool SowSeedsObjective::canSowAt(Area& area, BlockIndex block) const
{
	Faction* faction = area.m_actors.getFaction(m_actor);
	auto& blocks = area.getBlocks();
	return blocks.designation_has(block, *faction, BlockDesignation::SowSeeds) && !blocks.isReserved(block, *faction);
}
SowSeedsPathRequest::SowSeedsPathRequest(Area& area, SowSeedsObjective& objective) : ObjectivePathRequest(objective, true) // reserve block which passed predicate.
{
	bool unreserved = true;
	createGoAdjacentToDesignation(area, objective.m_actor, BlockDesignation::SowSeeds, objective.m_detour, unreserved, Config::maxRangeToSearchForHorticultureDesignations);
}
void SowSeedsPathRequest::onSuccess(Area& area, BlockIndex blockWhichPassedPredicate)
{
	static_cast<SowSeedsObjective&>(m_objective).select(area, blockWhichPassedPredicate);
}
