#include "givePlantsFluid.h"
#include "path.h"
#include "area.h"
void GivePlantsFluidEvent::execute()
{
	assert(m_objective.m_actor.m_canPickup.isCarryingFluidType(m_objective.m_plant->m_plantSpecies.fluidType));
	uint32_t quantity = std::min(m_objective.m_plant->getFluidDrinkVolume(), m_objective.m_actor.m_canPickup.getFluidVolume());
	m_objective.m_plant->addFluid(quantity, m_objective.m_actor.m_canPickup.getFluidType());
	m_objective.m_actor.m_canPickup.removeFluidVolume(quantity);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
GivePlantsFluidEvent::~GivePlantsFluidEvent() { m_objective.m_event.clearPointer(); }
void GivePlantsFluidThreadedTask::readStep()
{
	if(m_objective.m_plant == nullptr)
	{
		auto condition = [&](Block& block)
		{
			return block.m_hasDesignations.contains(*m_objective.m_actor.m_faction, BlockDesignation::GivePlantFluid);
		};
		m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
		return;
	}
	if(m_objective.m_actor.m_canPickup.isCarryingFluidType(m_objective.m_plant->m_plantSpecies.fluidType))
	{
		if(m_objective.m_actor.m_location == &m_objective.m_plant->m_location)
			m_objective.m_event.schedule(Config::givePlantsFluidDelaySteps, m_objective);
		else
			m_result = path::getForActor(m_objective.m_actor, m_objective.m_plant->m_location);
	}
	else
	{
		if(m_objective.m_actor.m_canPickup.isCarryingEmptyContainerWhichCanHoldFluid())
		{
			auto condition = [&](Block& block)
			{
				return m_objective.canFillAt(block);
			};
			m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
		}
		else
		{
			auto condition = [&](Block& block)
			{
				return block.m_hasItems.hasEmptyContainerWhichCanHoldFluidsCarryableBy(m_objective.m_actor) || block.m_hasItems.hasContainerContainingFluidTypeCarryableBy(m_objective.m_actor, m_objective.m_plant->m_plantSpecies.fluidType);
			};
			m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
		}
	}
}
void GivePlantsFluidThreadedTask::writeStep()
{
	if(m_result.empty() && !m_objective.m_event.exists())
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
	{
		assert(!m_objective.m_event.exists());
		if(!m_result.empty())
			m_objective.m_actor.m_canMove.setPath(m_result);
	}
}
bool GivePlantsFluidObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasGivePlantsFluidDesignations(*actor.m_faction);
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<GivePlantsFluidObjective>(actor);
}
void GivePlantsFluidObjective::execute()
{
	if(m_plant == nullptr)
	{
		m_plant = m_actor.m_location->m_area->m_hasFarmFields.getHighestPriorityPlantForGiveFluid(*m_actor.m_faction);
		if(m_plant != nullptr)
			execute();
		else
			if(m_actor.m_location->m_hasDesignations.contains(*m_actor.m_faction, BlockDesignation::GivePlantFluid))
			{
				Plant& plant = m_actor.m_location->m_hasPlant.get();
				if(m_actor.m_canPickup.isCarryingFluidType(m_plant->m_plantSpecies.fluidType))
				{
					m_plant = &plant;
					execute();
				}
				else
					m_threadedTask.create(*this);
			}
			else
				// Find a plant to give fluid to.
				m_threadedTask.create(*this);
	}
	else
	{
		if(m_actor.m_canPickup.isCarryingFluidType(m_plant->m_plantSpecies.fluidType))
		{
			if(m_actor.m_location == &m_plant->m_location)
				m_event.schedule(Config::givePlantsFluidDelaySteps, *this);
			else
				m_actor.m_canMove.setDestination(m_plant->m_location);
		}
		else
			// Find full container, empty contaner or fluid to fill empty container. 
			m_threadedTask.create(*this);
	}
}
bool GivePlantsFluidObjective::canFillAt(Block& block) const
{
	return getAdjacentBlockToFillAt(block) != nullptr || canFillItemAt(block);
}
Block* GivePlantsFluidObjective::getAdjacentBlockToFillAt(Block& block) const
{
	assert(m_plant != nullptr);
	for(Block* adjacent : block.m_adjacentsVector)
		if(adjacent->m_fluids.contains(&m_plant->m_plantSpecies.fluidType))
			return adjacent;
	return nullptr;
}
bool GivePlantsFluidObjective::canFillItemAt(Block& block) const
{
	return getItemToFillFromAt(block) != nullptr;
}
Item* GivePlantsFluidObjective::getItemToFillFromAt(Block& block) const
{
	assert(m_plant != nullptr);
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_hasCargo.getFluidType() == m_plant->m_plantSpecies.fluidType)
			return item;
	return nullptr;
}
