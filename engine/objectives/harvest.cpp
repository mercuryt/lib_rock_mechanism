#include "harvest.h"
#include "../area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../objective.h"
#include "../simulation.h"
#include <memory>
// Event.
HarvestEvent::HarvestEvent(Area& area, Step delay, HarvestObjective& ho, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_harvestObjective(ho) {}
void HarvestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->m_actors;
	assert(m_harvestObjective.m_block != BLOCK_INDEX_MAX);
	assert(actors.isAdjacentToLocation(m_harvestObjective.m_actor, m_harvestObjective.m_block));
	Blocks& blocks = area->getBlocks();
	if(!blocks.plant_exists(m_harvestObjective.m_block))
		// Plant no longer exsits, try again.
		m_harvestObjective.makePathRequest(*area);
	Plants& plants = area->m_plants;
	PlantIndex plant = blocks.plant_get(m_harvestObjective.m_block);
	ActorIndex actor = m_harvestObjective.m_actor;
	static const MaterialType& plantMatter = MaterialType::byName("plant matter");
	const ItemType& fruitItemType = plants.getSpecies(plant).harvestData->fruitItemType;
	Quantity quantityToHarvest = plants.getQuantityToHarvest(plant);
	if(quantityToHarvest == 0)
		actors.objective_canNotCompleteSubobjective(actor);
	else
	{
		uint32_t numberItemsHarvested = std::min(quantityToHarvest, actors.canPickUp_quantityWhichCanBePickedUpUnencombered(actor, fruitItemType, plantMatter));
		assert(numberItemsHarvested != 0);
		//TODO: apply horticulture skill.
		plants.harvest(plant, numberItemsHarvested);
		blocks.item_addGeneric(actors.getLocation(actor), fruitItemType, plantMatter, numberItemsHarvested);
		actors.objective_complete(actor, m_harvestObjective);
	}
}	
void HarvestEvent::clearReferences(Simulation&, Area*) { m_harvestObjective.m_harvestEvent.clearPointer(); } 
// Objective type.
bool HarvestObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return area.m_hasFarmFields.hasHarvestDesignations(*area.m_actors.getFaction(actor));
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Area& area, ActorIndex actor) const
{
	return std::make_unique<HarvestObjective>(area, actor);
}
// Objective.
HarvestObjective::HarvestObjective(Area& area, ActorIndex a) :
       	Objective(a, Config::harvestPriority), m_harvestEvent(area.m_eventSchedule) { }
/*
HarvestObjective::HarvestObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo), m_harvestEvent(deserializationMemo.m_simulation.m_eventSchedule), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_harvestEvent.schedule(Config::harvestEventDuration, *this, data["eventStart"].get<Step>());
	if(data.contains("block"))
		m_block = data["block"].get<BlockIndex>();
}
Json HarvestObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_block != BLOCK_INDEX_MAX)
		data["block"] = m_block;
	if(m_harvestEvent.exists())
		data["eventStart"] = m_harvestEvent.getStartStep();
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void HarvestObjective::execute(Area& area)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	if(m_block != BLOCK_INDEX_MAX)
	{
		//TODO: Area level listing of plant species to not harvest.
		if(actors.isAdjacentToLocation(m_actor, m_block) && blocks.plant_exists(m_block) && plants.readyToHarvest(blocks.plant_get(m_block)))
			begin(area);
		else
		{
			// Previously found block or route is no longer valid, redo from start.
			reset(area);
			execute(area);
		}
	}
	else
	{
		BlockIndex block = getBlockContainingPlantToHarvestAtLocationAndFacing(area, actors.getLocation(m_actor), actors.getFacing(m_actor));
		if(block != BLOCK_INDEX_MAX && blocks.plant_exists(m_block) && plants.readyToHarvest(blocks.plant_get(m_block)) && actors.move_tryToReserveOccupied(m_actor))
		{
			select(area, block);
			begin(area);
			return;
		}
		makePathRequest(area);
	}
}
void HarvestObjective::cancel(Area& area)
{
	
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	actors.move_pathRequestMaybeCancel(m_actor);
	m_harvestEvent.maybeUnschedule();
	if(m_block != BLOCK_INDEX_MAX && blocks.plant_exists(m_block) && plants.readyToHarvest(blocks.plant_get(m_block)))
		area.m_hasFarmFields.at(*actors.getFaction(m_actor)).addHarvestDesignation(blocks.plant_get(m_block));
}
void HarvestObjective::select(Area& area, BlockIndex block)
{
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	Actors& actors = area.getActors();
	assert(blocks.plant_exists(block));
	assert(plants.readyToHarvest(blocks.plant_get(block)));
	m_block = block;
	bool result = actors.canReserve_tryToReserveLocation(m_actor, block);
	assert(result);
	area.m_hasFarmFields.at(*actors.getFaction(m_actor)).removeHarvestDesignation(blocks.plant_get(block));
}
void HarvestObjective::begin(Area& area)
{
	Plants& plants = area.getPlants();
	assert(m_block != BLOCK_INDEX_MAX);
	assert(plants.readyToHarvest(area.getBlocks().plant_get(m_block)));
	m_harvestEvent.schedule(Config::harvestEventDuration, *this);
}
void HarvestObjective::reset(Area& area)
{
	cancel(area);
	m_block = BLOCK_INDEX_MAX;
}
void HarvestObjective::makePathRequest(Area& area)
{
	std::unique_ptr<PathRequest> pathRequest = std::make_unique<HarvestPathRequest>(area, *this);
	area.m_actors.move_pathRequestRecord(m_actor, std::move(pathRequest));
}
BlockIndex HarvestObjective::getBlockContainingPlantToHarvestAtLocationAndFacing(Area& area, BlockIndex location, Facing facing)
{
	Actors& actors = area.getActors();
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return blockContainsHarvestablePlant(area, block); };
	return actors.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(m_actor, location, facing, predicate);
}
bool HarvestObjective::blockContainsHarvestablePlant(Area& area, BlockIndex block) const
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	return blocks.plant_exists(block) && plants.readyToHarvest(blocks.plant_get(block)) && !blocks.isReserved(block, *actors.getFaction(m_actor));
}
HarvestPathRequest::HarvestPathRequest(Area& area, HarvestObjective& objective) : ObjectivePathRequest(objective, true) // reserve block which passed predicate.
{
	bool unreserved = true;
	createGoAdjacentToDesignation(area, objective.m_actor, BlockDesignation::Harvest, objective.m_detour, unreserved, Config::maxRangeToSearchForHorticultureDesignations);
}
void HarvestPathRequest::onSuccess(Area& area, BlockIndex blockWhichPassedPredicate)
{
	static_cast<HarvestObjective&>(m_objective).select(area, blockWhichPassedPredicate);
}
