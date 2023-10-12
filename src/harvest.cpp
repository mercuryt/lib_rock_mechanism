#include "harvest.h"
#include "area.h"
HarvestEvent::HarvestEvent(Step delay, HarvestObjective& ho, Block& b) : ScheduledEventWithPercent(ho.m_actor.getSimulation(), delay), m_harvestObjective(ho), m_block(b) {}
void HarvestEvent::execute()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return m_harvestObjective.blockContainsHarvestablePlant(block); };
	Block* plantBlock = m_harvestObjective.m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
	auto& actor = m_harvestObjective.m_actor;
	if(plantBlock == nullptr)
		actor.m_hasObjectives.cannotCompleteTask();
	else
	{
		Plant* plant = &plantBlock->m_hasPlant.get();
		static const MaterialType& plantMatter = MaterialType::byName("plant matter");
		const ItemType& fruitItemType = plant->m_plantSpecies.harvestData->fruitItemType;
		uint32_t maxHarvestItemsCanCarry = actor.m_canPickup.canPickupQuantityOf(fruitItemType, plantMatter);
		uint32_t numberItemsHarvested = std::min(maxHarvestItemsCanCarry, plant->m_quantityToHarvest);
		assert(numberItemsHarvested != 0);
		//TODO: apply horticulture skill.
		plant->harvest(numberItemsHarvested);
		actor.m_location->m_hasItems.add(fruitItemType, plantMatter, numberItemsHarvested);
		actor.m_hasObjectives.taskComplete();
	}
}	
void HarvestEvent::clearReferences() { m_harvestObjective.m_harvestEvent.clearPointer(); } 
bool HarvestObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasHarvestDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<HarvestObjective>(actor);
}
HarvestObjective::HarvestObjective(Actor& a) : Objective(Config::harvestPriority), m_actor(a), m_harvestEvent(a.getEventSchedule()) {}
void HarvestObjective::execute()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return blockContainsHarvestablePlant(block); };
	std::function<void(Block&)> callback = [&](Block& block) { m_harvestEvent.schedule(Config::harvestEventDuration, *this, block); };
	m_actor.m_canMove.goToPredicateBlockAndThen(predicate, callback);
}
void HarvestObjective::cancel()
{
	m_harvestEvent.maybeUnschedule();
}
bool HarvestObjective::blockContainsHarvestablePlant(const Block& block) const
{
	return block.m_hasPlant.exists() && block.m_hasPlant.get().readyToHarvest() && !block.m_reservable.isFullyReserved(m_actor.getFaction());
}
