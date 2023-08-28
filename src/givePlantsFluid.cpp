#include "givePlantsFluid.h"
#include "path.h"
#include "area.h"
#include "config.h"
GivePlantsFluidEvent::GivePlantsFluidEvent(Step delay, GivePlantsFluidObjective& gpfo) : ScheduledEventWithPercent(gpfo.m_actor.getSimulation(), delay), m_objective(gpfo) { }
void GivePlantsFluidEvent::execute()
{
	assert(m_objective.m_actor.m_canPickup.isCarryingFluidType(m_objective.m_plant->m_plantSpecies.fluidType));
	uint32_t quantity = std::min(m_objective.m_plant->getFluidDrinkVolume(), m_objective.m_actor.m_canPickup.getFluidVolume());
	m_objective.m_plant->addFluid(quantity, m_objective.m_actor.m_canPickup.getFluidType());
	m_objective.m_actor.m_canPickup.removeFluidVolume(quantity);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
void GivePlantsFluidEvent::clearReferences() { m_objective.m_event.clearPointer(); }
void GivePlantsFluidEvent::onCancel()
{
	Block& location = *m_objective.m_actor.m_location;
	location.m_area->m_hasFarmFields.at(*m_objective.m_actor.getFaction()).addGivePlantFluidDesignation(*m_objective.m_plant);
}
GivePlantsFluidThreadedTask::GivePlantsFluidThreadedTask(GivePlantsFluidObjective& gpfo) : PathToBlockBaseThreadedTask(gpfo.m_actor.getThreadedTaskEngine()), m_objective(gpfo), m_haulTool(nullptr) { }
void GivePlantsFluidThreadedTask::readStep()
{
	if(m_objective.m_plant == nullptr)
	{
		auto condition = [&](Block& block)
		{
			return block.m_hasDesignations.contains(*m_objective.m_actor.getFaction(), BlockDesignation::GivePlantFluid);
		};
		Block* plantLocation = path::getForActorToPredicateReturnEndOnly(m_objective.m_actor, condition);
		if(plantLocation == nullptr)
		{
			m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
			return;
		}
		assert(plantLocation->m_hasPlant.exists());
		//TODO: save plant in threaded task for now and only write to objective durring writeStep.
		m_objective.m_plant = &plantLocation->m_hasPlant.get();
	}
	if(m_objective.m_actor.m_canPickup.isCarryingFluidType(m_objective.m_plant->m_plantSpecies.fluidType))
	{
		if(m_objective.m_actor.m_location == &m_objective.m_plant->m_location)
			m_objective.m_event.schedule(Config::givePlantsFluidDelaySteps, m_objective);
		else
			pathTo(m_objective.m_actor, m_objective.m_plant->m_location);
	}
	else
	{
		if(m_objective.m_actor.m_canPickup.isCarryingEmptyContainerWhichCanHoldFluid())
		{
			auto condition = [&](Block& block) { return m_objective.canFillAt(block); };
			m_route = path::getForActorToPredicate(m_objective.m_actor, condition);
		}
		else
		{
			auto condition = [&](Block& block) { return m_objective.canGetFluidHaulingItemAt(block); };
			m_route = path::getForActorToPredicate(m_objective.m_actor, condition);
			m_haulTool = m_objective.getFluidHaulingItemAt(*m_route.back());
		}
	}
}
void GivePlantsFluidThreadedTask::writeStep()
{
	if(m_route.empty() && !m_objective.m_event.exists())
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
	{
		assert(!m_objective.m_event.exists());
		if(!m_route.empty())
			m_objective.m_actor.m_canMove.setPath(m_route);
	}
	if(m_haulTool)
		m_haulTool->m_reservable.reserveFor(m_objective.m_actor.m_canReserve, 1);
	cacheMoveCosts(m_objective.m_actor);
}
void GivePlantsFluidThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
bool GivePlantsFluidObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasGivePlantsFluidDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<GivePlantsFluidObjective>(actor);
}
GivePlantsFluidObjective::GivePlantsFluidObjective(Actor& a ) : Objective(Config::givePlantsFluidPriority), m_actor(a), m_plant(nullptr), m_event(m_actor.getEventSchedule()), m_threadedTask(m_actor.getThreadedTaskEngine()) { }
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute()
{
	if(m_plant == nullptr)
	{
		m_plant = m_actor.m_location->m_area->m_hasFarmFields.getHighestPriorityPlantForGiveFluid(*m_actor.getFaction());
		if(m_plant != nullptr)
			execute();
		else
			if(m_actor.m_location->m_hasDesignations.contains(*m_actor.getFaction(), BlockDesignation::GivePlantFluid))
			{
				Plant& plant = m_actor.m_location->m_hasPlant.get();
				if(m_actor.m_canPickup.isCarryingFluidType(m_plant->m_plantSpecies.fluidType))
				{
					// Standing in same block as a plant which needs fluid.
					m_plant = &plant;
					execute();
				}
				else
					// Get fluid for plant.
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
			{
				m_actor.m_location->m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeGivePlantFluidDesignation(*m_plant);
				m_event.schedule(Config::givePlantsFluidDelaySteps, *this);
			}
			else
				m_actor.m_canMove.setDestination(m_plant->m_location);
		}
		else
		{
			if(!m_actor.m_canPickup.isCarryingEmptyContainerWhichCanHoldFluid() && canGetFluidHaulingItemAt(*m_actor.m_location))
			{
				Item* item = getFluidHaulingItemAt(*m_actor.m_location);
				m_actor.m_canPickup.pickUp(*item, 1);
			}
			// Find full container, empty contaner or fluid to fill empty container. 
			m_threadedTask.create(*this);
		}
	}
}
bool GivePlantsFluidObjective::canFillAt(Block& block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getAdjacentBlockToFillAt(block) != nullptr || canFillFromItemAt(block);
}
Block* GivePlantsFluidObjective::getAdjacentBlockToFillAt(Block& block)
{
	assert(m_plant != nullptr);
	for(Block* adjacent : block.m_adjacentsVector)
		if(adjacent->m_fluids.contains(&m_plant->m_plantSpecies.fluidType))
			return adjacent;
	return nullptr;
}
bool GivePlantsFluidObjective::canFillFromItemAt(Block& block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(block) != nullptr;
}
Item* GivePlantsFluidObjective::getItemToFillFromAt(Block& block)
{
	assert(m_plant != nullptr);
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_hasCargo.getFluidType() == m_plant->m_plantSpecies.fluidType)
			return item;
	return nullptr;
}
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(Block& block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(block) != nullptr;
}
Item* GivePlantsFluidObjective::getFluidHaulingItemAt(Block& block)
{
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_itemType.canHoldFluids)
			return item;
	return nullptr;
}
