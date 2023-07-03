#include "harvest.h"
void HarvestEvent::execute()
{
	Plant* plant = getPlant();
	auto& actor = m_harvestObjective.m_actor;
	if(plant == nullptr)
		actor.actionComplete();

	uint32_t maxHarvestItemsCanCarry = (actor.m_carryWeight - actor.m_equipmentSet.m_totalMass) / plant.m_plantSpecies.harvestItemType.mass;
	uint32_t numberItemsHarvested = std::min(maxHarvestItemsCanCarry, plant.numberOfHarvestableItems());
	//TODO: apply horticulture skill.
	plant.harvest(numberItemsHarvested);
	if(actor.m_genericItems.contains(plant.m_plantSpecies.harvestItemType))
		actor.m_genericItems[plant.m_plantSpecies.harvestItemType] += numberItemsHarvested;
	else
		actor.m_genericItems[plant.m_plantSpecies.harvestItemType] = numberItemsHarvested;
}
Plant* HarvestEvent::getPlant()
{
	auto& block = *m_harvestObjective.m_actor.m_location;
	for(Plant* p : block.m_plants)
		if(p.readyToHarvest())
			return p;
	if(plant == nullptr)
		for(Block* adjacent : block.m_adjacentsVector)
			for(Plant* p : block.m_plants)
				if(p.readyToHarvest())
					return p;
	return nullptr;
}
~HarvestEvent::HarvestEvent() { m_harvestObjective.m_harvestEvent = nullptr; }
void HarvestThreadedTask::readStep()
{
	auto destinationCondition = [&](Block* block)
	{
		m_harvestObjective.canHarvestAt(*block);
	}
	m_result = path::getForActorToPredicate(destinationCondition);
}
void HarvestThreadedTask::writeStep()
{
	if(m_result.empty())
		m_harvestObjective.m_actor.CannotFulfillObjective();
	else
		m_harvestObjective.m_actor.setDestination(m_result);
}
bool HarvestObjectiveType::canBeAssigned(Actor& actor)
{
	return actor.m_location->m_area->m_hasFarmFields.areAnyPlantsDesignatedForHarvestInFieldsBeloningTo(actor.player);
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Actor& actor)
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
	for(Plant& plant : block.m_plants)
		if(plant.m_readyToHarvest)
			return true;
	for(Block* adjacent : block.m_adjacentsVector)
		for(Plant& plant : adjacent->m_plants)
			if(plant.m_readyToHarvest)
				return true;
	return false;
}
