#include "harvest.h"
#include "area.h"
void HarvestEvent::execute()
{
	Plant* plant = getPlant();
	auto& actor = m_harvestObjective.m_actor;
	if(plant == nullptr)
	{
		actor.m_hasObjectives.taskComplete();
		return;
	}
	static const MaterialType& plantMatter = MaterialType::byName("plantMatter");
	const ItemType& fruitItemType = plant->m_plantSpecies.harvestData->fruitItemType;
	uint32_t maxHarvestItemsCanCarry = actor.m_canPickup.canPickupQuantityOf(fruitItemType, plantMatter);
	uint32_t numberItemsHarvested = std::min(maxHarvestItemsCanCarry, plant->m_quantityToHarvest);
	assert(numberItemsHarvested != 0);
	//TODO: apply horticulture skill.
	plant->harvest(numberItemsHarvested);
	actor.m_location->m_hasItems.add(fruitItemType, plantMatter, numberItemsHarvested);
	actor.m_hasObjectives.taskComplete();
}	
Plant* HarvestEvent::getPlant()
{
	auto& block = *m_harvestObjective.m_actor.m_location;
	if(block.m_hasPlant.exists())
		if(block.m_hasPlant.get().readyToHarvest())
			return &block.m_hasPlant.get();
	for(Block* adjacent : block.m_adjacentsVector)
		if(adjacent->m_hasPlant.exists() && adjacent->m_hasPlant.get().readyToHarvest())
			return &adjacent->m_hasPlant.get();
	return nullptr;
}
HarvestEvent::~HarvestEvent() { m_harvestObjective.m_harvestEvent.clearPointer(); }
void HarvestThreadedTask::readStep()
{
	auto destinationCondition = [&](Block& block)
	{
		return m_harvestObjective.canHarvestAt(block);
	};
	m_result = path::getForActorToPredicate(m_harvestObjective.m_actor, destinationCondition);
}
void HarvestThreadedTask::writeStep()
{
	if(m_result.empty())
		m_harvestObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_harvestObjective);
	else
		m_harvestObjective.m_actor.m_canMove.setPath(m_result);
}
bool HarvestObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasHarvestDesignations(*actor.m_faction);
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<HarvestObjective>(actor);
}
void HarvestObjective::execute()
{
	if(canHarvestAt(*m_actor.m_location))
		//TODO: apply horticultural skill for harvest speed bonus.
		m_harvestEvent.schedule(Config::harvestEventDuration, *this);
	else
		m_threadedTask.create(*this);
}
bool HarvestObjective::canHarvestAt(Block& block) const
{
	if(block.m_hasPlant.exists() && block.m_hasPlant.get().readyToHarvest())
		return true;
	for(Block* adjacent : block.m_adjacentsVector)
		if(adjacent->m_hasPlant.exists() && adjacent->m_hasPlant.get().readyToHarvest())
			return true;
	return false;
}
