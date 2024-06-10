#include "harvest.h"
#include "../item.h"
#include "../area.h"
#include "../actor.h"
#include "../config.h"
#include "../farmFields.h"
#include "../objective.h"
#include "../plant.h"
#include "../simulation.h"
#include "types.h"
// Event.
HarvestEvent::HarvestEvent(Step delay, HarvestObjective& ho, const Step start) : ScheduledEvent(ho.m_actor.getSimulation(), delay, start), m_harvestObjective(ho) {}
void HarvestEvent::execute()
{
	assert(m_harvestObjective.m_block != BLOCK_INDEX_MAX);
	assert(m_harvestObjective.m_actor.isAdjacentTo(m_harvestObjective.m_block));
	auto& blocks = m_harvestObjective.m_actor.m_area->getBlocks();
	if(!blocks.plant_exists(m_harvestObjective.m_block))
		// Plant no longer exsits, try again.
		m_harvestObjective.m_threadedTask.create(m_harvestObjective);
	Plant& plant = blocks.plant_get(m_harvestObjective.m_block);
	Actor& actor = m_harvestObjective.m_actor;
	static const MaterialType& plantMatter = MaterialType::byName("plant matter");
	const ItemType& fruitItemType = plant.m_plantSpecies.harvestData->fruitItemType;
	if(!plant.m_quantityToHarvest)
		actor.m_hasObjectives.cannotCompleteSubobjective();
	uint32_t numberItemsHarvested = actor.m_canPickup.canPickupQuantityOf(fruitItemType, plantMatter, plant.m_quantityToHarvest);
	assert(numberItemsHarvested != 0);
	//TODO: apply horticulture skill.
	plant.harvest(numberItemsHarvested);
	blocks.item_addGeneric(actor.m_location, fruitItemType, plantMatter, numberItemsHarvested);
	actor.m_hasObjectives.objectiveComplete(m_harvestObjective);
}	
void HarvestEvent::clearReferences() { m_harvestObjective.m_harvestEvent.clearPointer(); } 
// Threaded task.
HarvestThreadedTask::HarvestThreadedTask(HarvestObjective& ho) : ThreadedTask(ho.m_actor.getThreadedTaskEngine()), m_harvestObjective(ho), m_findsPath(ho.m_actor, ho.m_detour) {}
void HarvestThreadedTask::readStep()
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return m_harvestObjective.blockContainsHarvestablePlant(block); };
	m_findsPath.m_maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_harvestObjective.m_actor.getFaction());
}
void HarvestThreadedTask::writeStep()
{
	assert(m_harvestObjective.m_block == BLOCK_INDEX_MAX);
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation)
		{
			m_harvestObjective.select(m_findsPath.getBlockWhichPassedPredicate());
			m_findsPath.reserveBlocksAtDestination(m_harvestObjective.m_actor.m_canReserve);
			m_harvestObjective.execute();
		}
		else
			m_harvestObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_harvestObjective);
	}
	else
	{
		if(!m_findsPath.areAllBlocksAtDestinationReservable(m_harvestObjective.m_actor.getFaction()))
			// There is no longer harvesting to be done at this destination or the desired place to stand has been reserved, try again.
			m_harvestObjective.m_threadedTask.create(m_harvestObjective);
		else
		{
			m_findsPath.reserveBlocksAtDestination(m_harvestObjective.m_actor.m_canReserve);
			m_harvestObjective.m_actor.m_canMove.setPath(m_findsPath.getPath());
			m_harvestObjective.select(m_findsPath.getBlockWhichPassedPredicate());
		}
	}
}
void HarvestThreadedTask::clearReferences() { m_harvestObjective.m_threadedTask.clearPointer(); }
// Objective type.
bool HarvestObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_area->m_hasFarmFields.hasHarvestDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<HarvestObjective>(actor);
}
// Objective.
HarvestObjective::HarvestObjective(Actor& a) :
       	Objective(a, Config::harvestPriority), m_harvestEvent(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine()) {}
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
void HarvestObjective::execute()
{
	auto& blocks = m_actor.m_area->getBlocks();
	if(m_block != BLOCK_INDEX_MAX)
	{
		//TODO: Area level listing of plant species to not harvest.
		if(m_actor.isAdjacentTo(m_block) && blocks.plant_exists(m_block) && blocks.plant_get(m_block).readyToHarvest())
			begin();
		else
		{
			// Previously found block or route is no longer valid, redo from start.
			reset();
			execute();
		}
	}
	else
	{
		if(m_actor.allOccupiedBlocksAreReservable(*m_actor.getFaction()))
		{
			BlockIndex block = getBlockContainingPlantToHarvestAtLocationAndFacing(m_actor.m_location, m_actor.m_facing);
			if(block != BLOCK_INDEX_MAX && blocks.plant_exists(m_block) && blocks.plant_get(m_block).readyToHarvest())
			{
				m_actor.reserveOccupied(m_actor.m_canReserve);
				select(block);
				begin();
				return;
			}
		}
		m_threadedTask.create(*this);
	}
}
void HarvestObjective::cancel()
{
	m_threadedTask.maybeCancel();
	m_harvestEvent.maybeUnschedule();
	auto& blocks = m_actor.m_area->getBlocks();
	if(m_block != BLOCK_INDEX_MAX && blocks.plant_exists(m_block) && blocks.plant_get(m_block).readyToHarvest())
		m_actor.m_area->m_hasFarmFields.at(*m_actor.getFaction()).addHarvestDesignation(blocks.plant_get(m_block));
	//TODO: This probably should be in hasObjectives instead of here, is this needed at all?
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
void HarvestObjective::select(BlockIndex block)
{
	auto& blocks = m_actor.m_area->getBlocks();
	assert(blocks.plant_exists(block));
	assert(blocks.plant_get(block).readyToHarvest());
	m_block = block;
	blocks.reserve(block, m_actor.m_canReserve);
	m_actor.m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeHarvestDesignation(blocks.plant_get(block));
}
void HarvestObjective::begin()
{
	assert(m_block != BLOCK_INDEX_MAX);
	assert(m_actor.m_area->getBlocks().plant_get(m_block).readyToHarvest());
	m_harvestEvent.schedule(Config::harvestEventDuration, *this);
}
void HarvestObjective::reset()
{
	cancel();
	m_block = BLOCK_INDEX_MAX;
}
BlockIndex HarvestObjective::getBlockContainingPlantToHarvestAtLocationAndFacing(BlockIndex location, Facing facing)
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return blockContainsHarvestablePlant(block); };
	return m_actor.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
}
bool HarvestObjective::blockContainsHarvestablePlant(BlockIndex block) const
{
	auto& blocks = m_actor.m_area->getBlocks();
	return blocks.plant_exists(block) && blocks.plant_get(block).readyToHarvest() && !blocks.isReserved(block, *m_actor.getFaction());
}
