#include "harvest.h"
#include "../area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../objective.h"
#include "../simulation.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../plants.h"
#include "../hasShapes.hpp"
#include "pathRequest.h"
#include <memory>
// Event.
HarvestEvent::HarvestEvent(const Step& delay, Area& area, HarvestObjective& ho, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_harvestObjective(ho)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
void HarvestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex();
	assert(m_harvestObjective.m_block.exists());
	assert(actors.isAdjacentToLocation(actor, m_harvestObjective.m_block));
	Blocks& blocks = area->getBlocks();
	if(!blocks.plant_exists(m_harvestObjective.m_block))
		// Plant no longer exsits, try again.
		m_harvestObjective.makePathRequest(*area, actor);
	Plants& plants = area->getPlants();
	PlantIndex plant = blocks.plant_get(m_harvestObjective.m_block);
	static MaterialTypeId plantMatter = MaterialType::byName("plant matter");
	ItemTypeId fruitItemType = PlantSpecies::getFruitItemType(plants.getSpecies(plant));
	Quantity quantityToHarvest = plants.getQuantityToHarvest(plant);
	if(quantityToHarvest == 0)
		actors.objective_canNotCompleteSubobjective(actor);
	else
	{
		Quantity numberItemsHarvested = std::min(quantityToHarvest, actors.canPickUp_quantityWhichCanBePickedUpUnencombered(actor, fruitItemType, plantMatter));
		assert(numberItemsHarvested != 0);
		//TODO: apply horticulture skill.
		plants.harvest(plant, numberItemsHarvested);
		blocks.item_addGeneric(actors.getLocation(actor), fruitItemType, plantMatter, numberItemsHarvested);
		actors.objective_complete(actor, m_harvestObjective);
	}
}	
void HarvestEvent::clearReferences(Simulation&, Area*) { m_harvestObjective.m_harvestEvent.clearPointer(); } 
// Objective type.
bool HarvestObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	return area.m_hasFarmFields.hasHarvestDesignations(area.getActors().getFactionId(actor));
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Area& area, const ActorIndex&) const
{
	return std::make_unique<HarvestObjective>(area);
}
// Objective.
HarvestObjective::HarvestObjective(Area& area) :
       	Objective(Config::harvestPriority), m_harvestEvent(area.m_eventSchedule) { }
HarvestObjective::HarvestObjective(const Json& data, Area& area) : 
	Objective(data), m_harvestEvent(area.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_harvestEvent.schedule(Config::harvestEventDuration, area, *this, data["actor"].get<ActorIndex>(), data["eventStart"].get<Step>());
	if(data.contains("block"))
		m_block = data["block"].get<BlockIndex>();
}
Json HarvestObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_block.exists())
		data["block"] = m_block;
	if(m_harvestEvent.exists())
		data["eventStart"] = m_harvestEvent.getStartStep();
	return data;
}
void HarvestObjective::execute(Area& area, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	if(m_block.exists())
	{
		//TODO: Area level listing of plant species to not harvest.
		if(actors.isAdjacentToLocation(actor, m_block) && blocks.plant_exists(m_block) && plants.readyToHarvest(blocks.plant_get(m_block)))
			begin(area, actor);
		else
		{
			// Previously found block or route is no longer valid, redo from start.
			reset(area, actor);
			execute(area, actor);
		}
	}
	else
	{
		BlockIndex block = getBlockContainingPlantToHarvestAtLocationAndFacing(area, actors.getLocation(actor), actors.getFacing(actor), actor);
		if(block.exists() && blocks.plant_exists(m_block) && plants.readyToHarvest(blocks.plant_get(m_block)) && actors.move_tryToReserveOccupied(actor))
		{
			select(area, block, actor);
			begin(area, actor);
			return;
		}
		makePathRequest(area, actor);
	}
}
void HarvestObjective::cancel(Area& area, const ActorIndex& actor)
{
	
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	actors.move_pathRequestMaybeCancel(actor);
	m_harvestEvent.maybeUnschedule();
	if(m_block.exists() && blocks.plant_exists(m_block) && plants.readyToHarvest(blocks.plant_get(m_block)))
		area.m_hasFarmFields.getForFaction(actors.getFactionId(actor)).addHarvestDesignation(blocks.plant_get(m_block));
}
void HarvestObjective::select(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	Actors& actors = area.getActors();
	assert(blocks.plant_exists(block));
	assert(plants.readyToHarvest(blocks.plant_get(block)));
	m_block = block;
	area.m_hasFarmFields.getForFaction(actors.getFactionId(actor)).removeHarvestDesignation(blocks.plant_get(block));
}
void HarvestObjective::begin(Area& area, const ActorIndex& actor)
{
	Plants& plants = area.getPlants();
	assert(m_block.exists());
	assert(plants.readyToHarvest(area.getBlocks().plant_get(m_block)));
	m_harvestEvent.schedule(Config::harvestEventDuration, area, *this, actor);
}
void HarvestObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_block.clear();
}
void HarvestObjective::makePathRequest(Area& area, const ActorIndex& actor)
{
	std::unique_ptr<PathRequest> pathRequest = std::make_unique<HarvestPathRequest>(area, *this, actor);
	area.getActors().move_pathRequestRecord(actor, std::move(pathRequest));
}
BlockIndex HarvestObjective::getBlockContainingPlantToHarvestAtLocationAndFacing(Area& area, const BlockIndex& location, Facing facing, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block) { return blockContainsHarvestablePlant(area, block, actor); };
	return actors.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
bool HarvestObjective::blockContainsHarvestablePlant(Area& area, const BlockIndex& block, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	return blocks.plant_exists(block) && plants.readyToHarvest(blocks.plant_get(block)) && !blocks.isReserved(block, actors.getFactionId(actor));
}
HarvestPathRequest::HarvestPathRequest(Area& area, HarvestObjective& objective, const ActorIndex& actor) : ObjectivePathRequest(objective, true) // reserve block which passed predicate.
{
	const bool unreserved = true;
	const bool reserve = true;
	createGoAdjacentToDesignation(area, actor, BlockDesignation::Harvest, objective.m_detour, unreserved, Config::maxRangeToSearchForHorticultureDesignations, reserve);
}
void HarvestPathRequest::onSuccess(Area& area, const BlockIndex& blockWhichPassedPredicate)
{
	static_cast<HarvestObjective&>(m_objective).select(area, blockWhichPassedPredicate, getActor());
}
Json HarvestPathRequest::toJson()
{
	Json output = PathRequest::toJson();
	output["type"] = "harvest";
	return output;
}