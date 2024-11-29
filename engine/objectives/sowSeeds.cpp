#include "sowSeeds.h"
#include "../area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../simulation.h"
#include "../hasShapes.hpp"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "designations.h"
#include "types.h"

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
	FactionId faction = actors.getFactionId(actor);
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
	return area.m_hasFarmFields.hasSowSeedsDesignations(area.getActors().getFactionId(actor));
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
BlockIndex SowSeedsObjective::getBlockToSowAt(Area& area, const BlockIndex& location, Facing facing, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
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
			FarmField* field = blocks.farm_get(m_block, actors.getFactionId(actor));
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
		if(actors.allOccupiedBlocksAreReservable(actor, actors.getFactionId(actor)))
		{
			BlockIndex block = getBlockToSowAt(area, actors.getLocation(actor), actors.getFacing(actor), actor);
			if(block.exists())
			{
				select(area, block, actor);
				bool result = actors.move_tryToReserveOccupied(actor);
				assert(result);
				begin(area, actor);
				return;
			}
		}
	}
	std::unique_ptr<PathRequest> pathRequest = std::make_unique<SowSeedsPathRequest>(area, *this, actor);
	actors.move_pathRequestRecord(actor, std::move(pathRequest));
}
void SowSeedsObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_event.maybeUnschedule();
	auto& blocks = area.getBlocks();
	FactionId faction = actors.getFactionId(actor);
	if(m_block.exists() && blocks.farm_contains(m_block, faction))
	{
		FarmField* field = blocks.farm_get(m_block, faction);
		if(field == nullptr)
			return;
		//TODO: check it is still planting season.
		if(blocks.plant_canGrowHereAtSomePointToday(m_block, field->plantSpecies))
			area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(m_block);
	}
}
void SowSeedsObjective::select(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	assert(!blocks.plant_exists(block));
	assert(blocks.farm_contains(block, actors.getFactionId(actor)));
	assert(m_block.empty());
	m_block = block;
	area.m_hasFarmFields.getForFaction(actors.getFactionId(actor)).removeSowSeedsDesignation(block);
}
void SowSeedsObjective::begin(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
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
	FactionId faction = actors.getFactionId(actor);
	auto& blocks = area.getBlocks();
	return blocks.designation_has(block, faction, BlockDesignation::SowSeeds) && !blocks.isReserved(block, faction);
}
SowSeedsPathRequest::SowSeedsPathRequest(Area& area, SowSeedsObjective& objective, const ActorIndex& actor) : ObjectivePathRequest(objective, true) // reserve block which passed predicate.
{
	bool unreserved = true;
	bool reserve = true;
	createGoAdjacentToDesignation(area, actor, BlockDesignation::SowSeeds, objective.m_detour, unreserved, Config::maxRangeToSearchForHorticultureDesignations, reserve);
}
void SowSeedsPathRequest::onSuccess(Area& area, const BlockIndex& blockWhichPassedPredicate)
{
	static_cast<SowSeedsObjective&>(m_objective).select(area, blockWhichPassedPredicate, getActor());
}
