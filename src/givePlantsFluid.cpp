#include "givePlantsFluid.h"
#include "area.h"
#include "config.h"
GivePlantsFluidEvent::GivePlantsFluidEvent(Step delay, GivePlantsFluidObjective& gpfo) : ScheduledEventWithPercent(gpfo.m_actor.getSimulation(), delay), m_objective(gpfo) { }
void GivePlantsFluidEvent::execute()
{
	assert(m_objective.m_actor.m_canPickup.isCarryingFluidType(m_objective.m_plantLocation->m_hasPlant.get().m_plantSpecies.fluidType));
	uint32_t quantity = std::min(m_objective.m_plantLocation->m_hasPlant.get().getVolumeFluidRequested(), m_objective.m_actor.m_canPickup.getFluidVolume());
	m_objective.m_plantLocation->addFluid(quantity, m_objective.m_actor.m_canPickup.getFluidType());
	m_objective.m_actor.m_canPickup.removeFluidVolume(quantity);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
void GivePlantsFluidEvent::clearReferences() { m_objective.m_event.clearPointer(); }
void GivePlantsFluidEvent::onCancel()
{
	Block& location = *m_objective.m_actor.m_location;
	location.m_area->m_hasFarmFields.at(*m_objective.m_actor.getFaction()).addGivePlantFluidDesignation(m_objective.m_plantLocation->m_hasPlant.get());
}
GivePlantsFluidThreadedTask::GivePlantsFluidThreadedTask(GivePlantsFluidObjective& gpfo) : ThreadedTask(gpfo.m_actor.getThreadedTaskEngine()), m_objective(gpfo), m_plantLocation(nullptr), m_findsPath(gpfo.m_actor) { }
void GivePlantsFluidThreadedTask::readStep()
{
	assert(m_objective.m_plantLocation == nullptr);
	const Faction* faction = m_objective.m_actor.getFaction();
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		if(block.m_hasDesignations.contains(*faction, BlockDesignation::GivePlantFluid))
		{
			m_plantLocation = const_cast<Block*>(&block);
			return true;
		}
		return false;
	};
	// TODO: Found path is not ever used, only tested for existance.
	m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *faction);
	if(!m_findsPath.found())
		return;
	assert(m_plantLocation != nullptr);
	assert(m_plantLocation->m_hasPlant.exists());
}
void GivePlantsFluidThreadedTask::writeStep()
{
	m_findsPath.cacheMoveCosts();
	assert(m_objective.m_plantLocation == nullptr);
	if(m_plantLocation == nullptr)
		// No plant accessable.
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
	{
		m_objective.m_plantLocation = m_plantLocation;
		m_objective.execute();
	}
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
GivePlantsFluidObjective::GivePlantsFluidObjective(Actor& a ) : Objective(Config::givePlantsFluidPriority), m_actor(a), m_plantLocation(nullptr), m_event(m_actor.getEventSchedule()), m_threadedTask(m_actor.getThreadedTaskEngine()) { }
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute()
{
	if(m_plantLocation == nullptr || !m_plantLocation->m_hasPlant.exists() || m_plantLocation->m_hasPlant.get().m_volumeFluidRequested == 0)
	{
		m_plantLocation = nullptr;
		// Get high priority job from area.
		Plant* plant = m_actor.m_location->m_area->m_hasFarmFields.getHighestPriorityPlantForGiveFluid(*m_actor.getFaction());
		if(plant != nullptr)
		{
			m_plantLocation = &plant->m_location;
			execute();
		}
		else
			// No high priority job found, find a suitable plant nearby.
			m_threadedTask.create(*this);
	}
	else
	{
		Plant& plant = m_plantLocation->m_hasPlant.get();
		if(m_actor.m_canPickup.isCarryingFluidType(plant.m_plantSpecies.fluidType))
		{
			// Has Fluid, go to plant.
			std::function<void(Block&)> callback = [&]([[maybe_unused]] const Block& block)
			{
				// Begin giving fluid.
				m_actor.m_location->m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeGivePlantFluidDesignation(plant);
				m_event.schedule(Config::givePlantsFluidDelaySteps, *this);
				execute();
			};
			m_actor.m_canMove.goToBlockAndThen(*m_plantLocation, callback);
		}
		else
		{
			if(m_actor.m_canPickup.isCarryingEmptyContainerWhichCanHoldFluid())
			{
				// Has empty Container, go to fill location.
				std::function<bool(const Block&)> predicate = [&](const Block& block) { return canFillAt(block); };
				std::function<void(Block&)> callback = [&](Block& fillFrom)
				{
					// Fill container.
					// TODO: delay.
					Item& haulTool = m_actor.m_canPickup.getItem();
					const FluidType& fluidType = plant.m_plantSpecies.fluidType;
					Item* fillFromItem = getItemToFillFromAt(fillFrom);
					Volume volume = haulTool.m_itemType.internalVolume;
					if(fillFromItem != nullptr)
					{
						assert(fillFromItem->m_hasCargo.getFluidType() == fluidType);
						volume = std::min(volume, fillFromItem->m_hasCargo.getFluidVolume());
						fillFromItem->m_hasCargo.removeFluidVolume(volume);
					}
					else
					{
						volume = std::min(volume, fillFrom.volumeOfFluidTypeContains(fluidType));
						fillFrom.removeFluid(volume, fluidType);
					}
					assert(volume != 0);
					haulTool.m_hasCargo.add(fluidType, volume);
					execute();
				};
				m_actor.m_canMove.goToPredicateBlockAndThen(predicate, callback);
			}
			else
			{
				// Does not have container.
				std::function<bool(const Block&)> predicate = [&](const Block& block) { return canGetFluidHaulingItemAt(block); };
				std::function<void(Block&)> callback = [&](Block& fillFrom)
				{
					Item* item = getFluidHaulingItemAt(fillFrom);
					m_actor.m_canPickup.pickUp(*item, 1u);
					execute();
				};
				m_actor.m_canMove.goToPredicateBlockAndThen(predicate, callback);
			}
		}
	}
}
void GivePlantsFluidObjective::cancel()
{
	m_event.maybeUnschedule();
	m_threadedTask.maybeCancel();
}
bool GivePlantsFluidObjective::canFillAt(const Block& block) const
{
	assert(m_plantLocation != nullptr);
	return block.volumeOfFluidTypeContains(m_plantLocation->m_hasPlant.get().m_plantSpecies.fluidType) != 0 || 
		const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(const_cast<Block&>(block)) != nullptr;
}
Item* GivePlantsFluidObjective::getItemToFillFromAt(Block& block)
{
	assert(m_plantLocation != nullptr);
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_hasCargo.getFluidType() == m_plantLocation->m_hasPlant.get().m_plantSpecies.fluidType)
				return item;
	return nullptr;
}
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(const Block& block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(const_cast<Block&>(block)) != nullptr;
}
Item* GivePlantsFluidObjective::getFluidHaulingItemAt(Block& block)
{
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_itemType.canHoldFluids && m_actor.m_canPickup.canPickupAny(*item))
			return item;
	return nullptr;
}
