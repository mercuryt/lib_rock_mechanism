#include "harvest.h"
#include "area.h"
// Event.
HarvestEvent::HarvestEvent(Step delay, HarvestObjective& ho) : ScheduledEventWithPercent(ho.m_actor.getSimulation(), delay), m_harvestObjective(ho) {}
void HarvestEvent::execute()
{
	assert(m_harvestObjective.m_block != nullptr);
	assert(m_harvestObjective.m_actor.isAdjacentTo(*m_harvestObjective.m_block));
	if(!m_harvestObjective.m_block->m_hasPlant.exists())
		// Plant no longer exsits, try again.
		m_harvestObjective.m_threadedTask.create(m_harvestObjective);
	Plant& plant = m_harvestObjective.m_block->m_hasPlant.get();
	Actor& actor = m_harvestObjective.m_actor;
	static const MaterialType& plantMatter = MaterialType::byName("plant matter");
	const ItemType& fruitItemType = plant.m_plantSpecies.harvestData->fruitItemType;
	uint32_t maxHarvestItemsCanCarry = actor.m_canPickup.canPickupQuantityOf(fruitItemType, plantMatter);
	uint32_t numberItemsHarvested = std::min(maxHarvestItemsCanCarry, plant.m_quantityToHarvest);
	assert(numberItemsHarvested != 0);
	//TODO: apply horticulture skill.
	plant.harvest(numberItemsHarvested);
	actor.m_location->m_hasItems.add(fruitItemType, plantMatter, numberItemsHarvested);
	actor.m_hasObjectives.objectiveComplete(m_harvestObjective);
}	
void HarvestEvent::clearReferences() { m_harvestObjective.m_harvestEvent.clearPointer(); } 
// Threaded task.
HarvestThreadedTask::HarvestThreadedTask(HarvestObjective& ho) : ThreadedTask(ho.m_actor.getThreadedTaskEngine()), m_harvestObjective(ho), m_findsPath(ho.m_actor, ho.m_detour) {}
void HarvestThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return m_harvestObjective.blockContainsHarvestablePlant(block); };
	m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_harvestObjective.m_actor.getFaction());
}
void HarvestThreadedTask::writeStep()
{
	assert(m_harvestObjective.m_block == nullptr);
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation)
		{
			m_harvestObjective.select(*m_findsPath.getBlockWhichPassedPredicate());
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
			m_harvestObjective.select(*m_findsPath.getBlockWhichPassedPredicate());
		}
	}
}
void HarvestThreadedTask::clearReferences() { m_harvestObjective.m_threadedTask.clearPointer(); }
// Objective type.
bool HarvestObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasHarvestDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<HarvestObjective>(actor);
}
// Objective.
HarvestObjective::HarvestObjective(Actor& a) : Objective(Config::harvestPriority), m_actor(a), m_block(nullptr), m_harvestEvent(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine()) {}
void HarvestObjective::execute()
{
	if(m_block != nullptr)
	{
		if(m_actor.isAdjacentTo(*m_block))
			begin();
		else
		{
			// Previously found route is no longer valid, redo from start.
			reset();
			execute();
		}
	}
	else
	{
		if(m_actor.allOccupiedBlocksAreReservable(*m_actor.getFaction()))
		{
			Block* block = getBlockContainingPlantToHarvestAtLocationAndFacing(*m_actor.m_location, m_actor.m_facing);
			if(block != nullptr)
			{
				m_actor.reserveOccupied(m_actor.m_canReserve);
				select(*block);
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
}
void HarvestObjective::select(Block& block)
{
	assert(block.m_hasPlant.exists());
	assert(block.m_hasPlant.get().readyToHarvest());
	m_block = &block;
	block.m_reservable.reserveFor(m_actor.m_canReserve, 1u);
	block.m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeHarvestDesignation(block.m_hasPlant.get());
}
void HarvestObjective::begin()
{
	assert(m_block != nullptr);
	assert(m_block->m_hasPlant.get().readyToHarvest());
	m_harvestEvent.schedule(Config::harvestEventDuration, *this);
}
void HarvestObjective::reset()
{
	m_actor.m_canReserve.clearAll();
	m_block = nullptr;
}
Block* HarvestObjective::getBlockContainingPlantToHarvestAtLocationAndFacing(const Block& location, Facing facing)
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return blockContainsHarvestablePlant(block); };
	return m_actor.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
}
bool HarvestObjective::blockContainsHarvestablePlant(const Block& block) const
{
	return block.m_hasPlant.exists() && block.m_hasPlant.get().readyToHarvest() && !block.m_reservable.isFullyReserved(m_actor.getFaction());
}
